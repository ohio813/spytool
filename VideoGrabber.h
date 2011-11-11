#pragma once
#include "dataprovider.h"
#include <windows.h>
#include <vfw.h>

class VideoGrabber : public DataProvider
{
private:
	static const int MOTION_TRESHOLD = 100; // number of differed pixels from frame to frame for motion detection
	// if two grayscaled pixels differs by this (or bigger) value, these pixels are different
	static const int PIXELS_DIFFERENCE_TRESHOLD = 10; 
	HWND camhwnd;
	BOOL mIsFrameGrabbed;
	PBITMAPINFO mBitmapInfo;
	PBYTE mPreviousFrame;
	HANDLE mListenerHandle;
	DWORD mThreadID;
	BOOL mMotionDetectedDuringLastSecond;
	void GrabFrame();

	friend DWORD WINAPI ListeningRoutine(LPVOID lpParam);
	friend LRESULT CALLBACK FrameCallbackProc(HWND hWnd, LPVIDEOHDR lpVHdr);

protected:
	PSTR GetName();
	PSTR GetExtension();

public:
	void Init();
	void Finalize();
	VideoGrabber(void);
	~VideoGrabber(void);
};
