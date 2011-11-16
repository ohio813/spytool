#include "VideoGrabber.h"
#include "DataAccumulator.h"
#include <windows.h>
#include <vfw.h>
#include <tchar.h>

const int FRAME_WIDTH = 300;
const int FRAME_HEIGHT = 300;

VideoGrabber::VideoGrabber(void)
{
	mPreviousFrame = NULL;
	mBitmapInfo = NULL;
	mIsFrameGrabbed = false;

	mListenerHandle = CreateThread(0, 0, ListeningRoutine, this, CREATE_SUSPENDED, &mThreadID);
}

VideoGrabber::~VideoGrabber(void)
{
	CloseHandle(mListenerHandle);
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
	// Setup capture window and connect webcam driver
	camhwnd = capCreateCaptureWindow (_T("Ahtung!"), 0 , 0, 0, FRAME_WIDTH, FRAME_HEIGHT, 0, 0);
	SendMessage(camhwnd, WM_CAP_DRIVER_CONNECT, 0, 0);
	capSetCallbackOnFrame(camhwnd, FrameCallbackProc);
	capSetUserData(camhwnd, this); // Callback functions may use pointer to this VideoGrabber

	if (mPreviousFrame != NULL)
		delete[] mPreviousFrame;
	mPreviousFrame = NULL;

	mMotionDetectedDuringLastSecond = FALSE;

	// Setup video capturing and streaming
	CAPTUREPARMS parms;
	parms.fAbortLeftMouse = FALSE;
	parms.fAbortRightMouse = FALSE;
	parms.fLimitEnabled = TRUE;
	parms.wTimeLimit = 0; // TODO!
	parms.fYield = TRUE; // TODO!

	capCaptureSetSetup(camhwnd, &parms, sizeof(parms));
}

void VideoGrabber::Finalize()
{
	SendMessage(camhwnd, WM_CAP_STOP, 0, 0);
	SendMessage(camhwnd, WM_CAP_DRIVER_DISCONNECT, 0, 0);

	SetEnabled(FALSE);
}

DWORD WINAPI ListeningRoutine(LPVOID lpParam) 
{
	BOOL isCapturing = FALSE;
	VideoGrabber* videoGrabber = (VideoGrabber*)lpParam;
	static PTSTR captureFileName;
	
	while (videoGrabber->IsEnabled())
	{
		videoGrabber->GrabFrame();

		if (videoGrabber->mMotionDetectedDuringLastSecond) 
		{
			if (!isCapturing)
			{
				captureFileName = videoGrabber->GetNewDataFileName();
				// MakeSureDirectiryPathExists(captureFileName);
				capFileSetCaptureFile(videoGrabber->camhwnd, captureFileName);
				capCaptureSequence(videoGrabber->camhwnd);
			}
		} 
		else if (isCapturing)
		{
			capCaptureStop(videoGrabber->camhwnd);
			videoGrabber->GetDataAccumulator()->LogVideo(captureFileName);
		}

		Sleep(1000); 
	}
	return FALSE;
}

void VideoGrabber::GrabFrame()
{
	// Grab a Frame and continue to capture (don't stop)
	// After the frame is grabbed a callback function is called (FrameCallbackProc)
	SendMessage(camhwnd, WM_CAP_GRAB_FRAME_NOSTOP, 0, 0);
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
	static PBYTE currentFrameGS;
	static PBYTE currentFrameBlurred;
	// If no data provided by driver - nothing to do
	if (lpVHdr->dwBytesUsed == 0) return FALSE;
	
	int grayScaleSize = lpVHdr->dwBytesUsed/3; // RGB uses 24 BPP, GS is 8 BPP

	// Get pointer to our video grabber - remember, this is friend function
	VideoGrabber* videoGrabber = (VideoGrabber*) capGetUserData(hWnd);
	if (videoGrabber->mIsFrameGrabbed)
	{
		// Get video format from driver (including resolution)
		if (videoGrabber->mBitmapInfo == NULL) 
		{
			DWORD videoFormatSize = capGetVideoFormatSize(videoGrabber->camhwnd);
			videoGrabber->mBitmapInfo = (PBITMAPINFO) new char[videoFormatSize];	
			capGetVideoFormat(videoGrabber->camhwnd, videoGrabber->mBitmapInfo, videoFormatSize);
			currentFrameGS = new BYTE[grayScaleSize];
			currentFrameBlurred = new BYTE[grayScaleSize];
		}

		ApplyGrayScaleFilter(lpVHdr, currentFrameGS); // Pass current frame data to grayscale it
		// Blurring decreases noise. mBitmapInfo contains frame dimensions (width & height)
		ApplyAverageBlurFilter(currentFrameGS, videoGrabber->mBitmapInfo, currentFrameBlurred);

		if (videoGrabber->mPreviousFrame != NULL)
		{
			// Calculate difference between frames
			int differedPixelsNum = CompareFrames(currentFrameBlurred, videoGrabber->mPreviousFrame, 
				videoGrabber->mBitmapInfo, videoGrabber->PIXELS_DIFFERENCE_TRESHOLD);
			videoGrabber->mMotionDetectedDuringLastSecond = 
				(differedPixelsNum > videoGrabber->MOTION_TRESHOLD); // Motion detected!
		}

		memcpy(videoGrabber->mPreviousFrame, currentFrameBlurred, grayScaleSize);
		videoGrabber->mIsFrameGrabbed = FALSE;
	}

	return TRUE;
}

