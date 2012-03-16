#include "VideoGrabber.h"
#include "DataAccumulator.h"
#include <windows.h>
#include <vfw.h>
#include <tchar.h>
#include <DbgHelp.h>
#include <WinUser.h>

const int FRAME_WIDTH = 300;
const int FRAME_HEIGHT = 300;

VideoGrabber::VideoGrabber(void)
{
	mPreviousFrame = NULL;
	mBitmapInfo = NULL;
	mCurrentFrameGS = NULL;
	mCurrentFrameBlurred = NULL;
	// No security opts, automatic reset, initial state: unsignaled, unnamed
	mFrameProcessedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

VideoGrabber::~VideoGrabber(void)
{
	delete[] mBitmapInfo;
	if (mCurrentFrameGS != NULL) {
		delete[] mCurrentFrameGS;
	}
	if (mCurrentFrameBlurred != NULL) {
		delete[] mCurrentFrameBlurred;
	}
	if (mPreviousFrame != NULL) {
		delete[] mPreviousFrame;
	}
	if (mBitmapInfo != NULL) {
		delete mBitmapInfo;
	}
	CloseHandle(mFrameProcessedEvent);
}

PSTR VideoGrabber::GetName()
{
	return "moves";
};

PSTR VideoGrabber::GetExtension()
{
	return "avi";
};

void VideoGrabber::Init()
{
	mGrabNextFrame = FALSE;
	mPreviousFrameExists = FALSE;

	// Setup capture window and connect webcam driver
	camhwnd = capCreateCaptureWindow (_T("Ahtung!"), 0 , 0, 0, FRAME_WIDTH, FRAME_HEIGHT, 0, 0);
	SendMessage(camhwnd, WM_CAP_DRIVER_CONNECT, 0, 0);
	capSetCallbackOnFrame(camhwnd, FrameCallbackProc);
	capSetCallbackOnVideoStream(camhwnd, FrameCallbackProc); // Use same callback function, consider mGrabNextFrame flag!
	capSetUserData(camhwnd, this); // Callback functions may use pointer to this VideoGrabber

	if (mPreviousFrame != NULL) {
		delete[] mPreviousFrame; 
		mPreviousFrame = NULL;
	}

	mMotionDetectedDuringLastSecond = FALSE;

	// TODO: Use MPEGLAYER3WAVEFORMAT instead this
	// Setup audio params
	WAVEFORMATEX wfex;

	wfex.wFormatTag = WAVE_FORMAT_PCM;
	wfex.nChannels = 1;                // Use mono
	wfex.nSamplesPerSec = 8000;
	wfex.nAvgBytesPerSec = 8000;
	wfex.nBlockAlign = 1;
	wfex.wBitsPerSample = 8;
	wfex.cbSize = 0;
	capSetAudioFormat(camhwnd, &wfex, sizeof(WAVEFORMATEX)); 

	// Setup video capturing and streaming
	CAPTUREPARMS parms;
	capCaptureGetSetup(camhwnd, &parms, sizeof(CAPTUREPARMS));

	parms.fAbortLeftMouse = FALSE;
	parms.wPercentDropForError = 100; // Never abort capturing in case of dropped frames
	parms.fAbortRightMouse = FALSE;
	//parms.fLimitEnabled = TRUE;
	//parms.wTimeLimit = 0; // TODO!
	parms.fYield = TRUE; // TODO!
	capCaptureSetSetup(camhwnd, &parms, sizeof(parms));

	// !!!
	capSetCallbackOnError(camhwnd, capErrorCallback);

	// Resume thread for motion detection
	mListenerHandle = CreateThread(0, 0, ListeningRoutine, this, CREATE_SUSPENDED, &mThreadID);
	SetEnabled(TRUE);
	ResumeThread(mListenerHandle);
}

LRESULT CALLBACK capErrorCallback(HWND hWnd, int nID, LPCTSTR lpsz) {
	CHAR errorMsg[100];
	wsprintf(errorMsg, "Error message: %s\nError Code: %d", lpsz, nID);
	MessageBox(NULL, errorMsg, "CAP ERROR", MB_OK | MB_ICONERROR);
	return 0;
}

void VideoGrabber::Finalize()
{
	const int LISTENING_THREAD_TIMEOUT = 3000;

	SendMessage(camhwnd, WM_CAP_STOP, 0, 0);
	SendMessage(camhwnd, WM_CAP_DRIVER_DISCONNECT, 0, 0);

	// Terminate listening thread
	SetEnabled(FALSE);
	WaitForSingleObject(mListenerHandle, LISTENING_THREAD_TIMEOUT);
	CloseHandle(mListenerHandle);
}

DWORD WINAPI ListeningRoutine(LPVOID lpParam) 
{
	const int SAMPLING_FRAME_RATE = 1; // per second

	BOOL isCapturing = FALSE;
	VideoGrabber* videoGrabber = (VideoGrabber*)lpParam;
	static PTSTR captureFileName;
	
	while (videoGrabber->IsEnabled())
	{
		videoGrabber->GrabFrame(isCapturing);
		int recode;
		if (videoGrabber->mMotionDetectedDuringLastSecond) 
		{
			if (!isCapturing)
			{
				captureFileName = videoGrabber->GetNewDataFileName();
				MakeSureDirectoryPathExists(captureFileName);
				recode = capFileSetCaptureFile(videoGrabber->camhwnd, captureFileName);
				recode = capCaptureSequence(videoGrabber->camhwnd);
				isCapturing = TRUE;
			}
		} 
		else if (isCapturing)
		{
			// ISSUE: Sometimes motion detected when there is no motion in the scene actually.
			//        This is caused by automatic aperture enabled in webcam: the cam adjusting
			//        its aperture to avoid under/overexposition. Hence brightness of pixels is 
			//        changed and motion detection mechanism makes false alarm. However this can
			//        be patched because the effect causes capture of 2-3 sec. video stream. So
			//        quick and dirty solution would be to filter all streams shorter than 4 sec.
			recode = capCaptureStop(videoGrabber->camhwnd);
			recode = GetLastError();
			videoGrabber->GetDataAccumulator()->LogVideo(captureFileName);
			isCapturing = FALSE;
		}

		Sleep(1000/SAMPLING_FRAME_RATE); 
	}
	return FALSE;
}

void VideoGrabber::GrabFrame(BOOL isInCapturingState)
{
	const int FRAME_PROCESSING_TIMEOUT = 1000; // 1 second

	// Indicate we are wanting to grab mext frame.
	mGrabNextFrame = TRUE;

	// We need to distinguish capturing state because during capturing it is impossible 
	// to grab single frames. This is done in CallBackOnVideoStream instead which in our
	// case is the same callback function as CallbackOnGrabFrame.
	if (!isInCapturingState) {
		// After the frame is grabbed a callback function is called (FrameCallbackProc)
		SendMessage(camhwnd, WM_CAP_GRAB_FRAME_NOSTOP, 0, 0);
	}
	// When IsInCapturingState the callback function is called for each frame (30 fps)

	// Need to wait here until the frame is processed and possible motion detected
	DWORD result = WaitForSingleObject(mFrameProcessedEvent, FRAME_PROCESSING_TIMEOUT);
}

static void ApplyGrayScaleFilter(LPVIDEOHDR frameHeader, PBYTE targetBuffer) 
{
	PBYTE rawFrame = frameHeader->lpData;
	int grayedSize = frameHeader->dwBytesUsed/3;

	for (int i = 0; i<grayedSize; i++)
	{
		targetBuffer[i] = (BYTE)(rawFrame[3*i]*0.299f + rawFrame[3*i+1]*0.587f + rawFrame[3*i+2]*0.114f);
	}
}

static inline BYTE GetPixelAt(PBYTE frame, int x, int y, int width)
{
	return frame[y*width + x];
}

static void ApplyAverageBlurFilter(PBYTE frameData, PBITMAPINFO frameInfo, PBYTE targetBuffer) 
{
	LONG width = frameInfo->bmiHeader.biWidth;
	LONG height = frameInfo->bmiHeader.biHeight;

	for (int x=1; x < width-1; x++)
	{
		for (int y=1; y < height-1; y++)
		{
			// Calculate average of all pixels in a 3x3 rectangle around (x, y)
			targetBuffer[x + y*width] = (
				GetPixelAt(frameData, x-1, y-1, height) + GetPixelAt(frameData, x, y-1, height) + GetPixelAt(frameData, x+1, y-1, height) + 
				GetPixelAt(frameData, x-1, y, height) + GetPixelAt(frameData, x, y, height) + GetPixelAt(frameData, x+1, y, height) + 
				GetPixelAt(frameData, x-1, y+1, height) + GetPixelAt(frameData, x, y+1, height) + GetPixelAt(frameData, x+1, y+1, height)
				) / 9;
		}
	}
}

static int CompareFrames(PBYTE frameA, PBYTE frameB, PBITMAPINFO frameInfo, int pixels_diff_treshold)
{
	int differedPixels = 0;
	int frameLength = frameInfo->bmiHeader.biHeight * frameInfo->bmiHeader.biWidth;
	for (int i = 0; i<frameLength; i++)
	{
		if (abs(frameA[i] - frameB[i]) > pixels_diff_treshold)
			differedPixels++;
	}

	return differedPixels;
}

// Frame data is stored in lpVHdr 
static LRESULT CALLBACK FrameCallbackProc(HWND hWnd, LPVIDEOHDR lpVHdr)
{
	// If no data provided by driver (dropped frame) - nothing to do
	if (lpVHdr->dwBytesUsed == 0) return FALSE;
	
	int grayScaleSize = lpVHdr->dwBytesUsed/3; // RGB uses 24 BPP, GS is 8 BPP

	// Get pointer to our video grabber - remember, this is friend function
	VideoGrabber* videoGrabber = (VideoGrabber*) capGetUserData(hWnd);
	if (videoGrabber->mGrabNextFrame)
	{
		// Get video format from driver (including resolution)
		if (videoGrabber->mBitmapInfo == NULL) 
		{
			// All these lines are run only once! I put them here and not in the constructor \
			   because I need to run them in context of callback. Strange though...
			DWORD videoFormatSize = capGetVideoFormatSize(videoGrabber->camhwnd);
			videoGrabber->mBitmapInfo = (PBITMAPINFO) new char[videoFormatSize];	
			capGetVideoFormat(videoGrabber->camhwnd, videoGrabber->mBitmapInfo, videoFormatSize);
			videoGrabber->mCurrentFrameGS = new BYTE[grayScaleSize];
			videoGrabber->mCurrentFrameBlurred = new BYTE[grayScaleSize];
			videoGrabber->mPreviousFrame = new BYTE[grayScaleSize];
		}

		ApplyGrayScaleFilter(lpVHdr, videoGrabber->mCurrentFrameGS); // Pass current frame data to grayscale it
		// Blurring decreases noise. mBitmapInfo contains frame dimensions (width & height)
		ApplyAverageBlurFilter(videoGrabber->mCurrentFrameGS, videoGrabber->mBitmapInfo, videoGrabber->mCurrentFrameBlurred);

		if (videoGrabber->mPreviousFrameExists)
		{
			// Calculate difference between frames
			int differedPixelsNum = CompareFrames(videoGrabber->mCurrentFrameBlurred, videoGrabber->mPreviousFrame, 
				videoGrabber->mBitmapInfo, videoGrabber->PIXELS_DIFFERENCE_TRESHOLD);
			videoGrabber->mMotionDetectedDuringLastSecond = 
				(differedPixelsNum > videoGrabber->MOTION_TRESHOLD); // Motion detected!
		}

		memcpy(videoGrabber->mPreviousFrame, videoGrabber->mCurrentFrameBlurred, grayScaleSize);
		videoGrabber->mPreviousFrameExists = TRUE;		// Now we have frame to compare with
		videoGrabber->mGrabNextFrame = FALSE;			// frame for current second has been processed
		SetEvent(videoGrabber->mFrameProcessedEvent);	// Signal about frame processing completion
	}

	return TRUE;
}

