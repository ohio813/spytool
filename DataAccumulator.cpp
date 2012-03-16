#include <Windows.h>
#include <stdio.h>
#include <DbgHelp.h> // for MakeSureDirectoryPathExists(fileName);
#include "DataAccumulator.h"
#include "DataProvider.h"
#include "FileSystemEntity.h"
#include "resource.h" // for getting HTML log header

const int DataAccumulator::BUFFER_SIZE = 256;//8192; // 4 pages: 8K unicode chars
const int DataAccumulator::LOG_FILE_NAME_LENGTH = 100;

const PSTR const DataAccumulator::DATA_FILE_NAME = "log.html";
const PWSTR const DataAccumulator::SECTION_END_TAGS = L"</p>\n\t\t\t\t\t</td>\n\t\t\t\t</tr>\n";

DataAccumulator::DataAccumulator(void)
{
	mPreviousRowExists = false;
	mPreviousParagraphExists = false;
	mCurrentChunkLength = 0;
	chunk = new WCHAR[BUFFER_SIZE];
	memset(chunk, 0, BUFFER_SIZE * sizeof(WCHAR));
	mIsFirstLog = TRUE;
	mCurrentLogFile = new CHAR[LOG_FILE_NAME_LENGTH];
	mIsLogFileGenerated = FALSE;
	mLogLockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
}

DataAccumulator::~DataAccumulator(void)
{
	FlushChunk(); // write remaining log info to file
	delete[] chunk;
	delete[] mCurrentLogFile;
}

void DataAccumulator::LogKey(PWSTR key)
{
	Log2HTML(key);
}

void DataAccumulator::LogVideo(PTSTR videoFile)
{
	// Convert file name from ANSII to Unicode
	int fnLength = strlen(videoFile) + 1;
	PWSTR wideFileName = new WCHAR[fnLength];
	MultiByteToWideChar(CP_ACP, 0, videoFile, fnLength, wideFileName, fnLength);

	// Append HTML tags
	// NOTE: HTML 5 doesn't support AVI mime file. webcam CAP services cannot write other than AVI format
	//       Hence write video to log as link
	//const PWSTR const HTML_VIDEO_START = L"<video controls=\"controls\"><source src=\"";
	//const PWSTR const HTML_VIDEO_END = L"\" type=\"video/msvideo\" /></video>";
	const PWSTR const HTML_VIDEO_START = L"<a href=\"";
	const PWSTR const HTML_VIDEO_END = L"\">video</a>";
	PWSTR logEntry = new WCHAR[wcslen(HTML_VIDEO_START) + fnLength + wcslen(HTML_VIDEO_END)];
	wcscpy(logEntry, HTML_VIDEO_START);
	wcscat(logEntry, wideFileName);
	wcscat(logEntry, HTML_VIDEO_END);

	// Log video in HTML
	Log2HTML(logEntry);

	// Free used memory
	delete[] logEntry;
	delete[] wideFileName;
}

void DataAccumulator::LogPrintScreen(PTSTR imageFile)
{
}

void DataAccumulator::Append2Log(PWSTR newAppendee) {
	mCurrentChunkLength += wcslen(newAppendee);
	wcscat(chunk, newAppendee);
}

void DataAccumulator::Log2HTML(PWSTR htmlString) {
	const int TIMEOUT = 250; // 1/4 sec
	static SYSTEMTIME lastLogSectionTime;

	// This method is synchronized!
	WaitForSingleObject(mLogLockEvent, TIMEOUT);
	{
		int elapsedSeconds = GetElapsedSecondsFromLastLog();

		// If appendee is nearly full or log started yesterday, flush it to disk
		if (mIsFirstLog || (mCurrentChunkLength > 0.9 * BUFFER_SIZE)) {
			FlushChunk();
			mCurrentChunkLength = 0;
		}

		if (elapsedSeconds > 60*5) // 5 min.
		{
			if (mPreviousRowExists) {
				Append2Log(SECTION_END_TAGS);
			}
			Append2Log(L"\t\t\t\t<tr>\n\t\t\t\t\t<td>");

			SYSTEMTIME time;
			GetSystemTime(&time);
			lastLogSectionTime = time; // Rmember time of section opening 
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
	SetEvent(mLogLockEvent);
}

int DataAccumulator::GetElapsedSecondsFrom(FILETIME* date, BOOL updateDate) {
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

	// Get date parameter stored in LARGE_INTEGER unit for further subtraction
	LARGE_INTEGER liDate;
	liDate.HighPart = date->dwHighDateTime;
	liDate.LowPart = date->dwLowDateTime;

	// Calculate difference in seconds.
	seconds = (liNow.QuadPart - liDate.QuadPart) / PERIODS_IN_SEC;

	// Save current date/time to date?
	if (updateDate) {
		date->dwHighDateTime = ftNow.dwHighDateTime;
		date->dwLowDateTime = ftNow.dwLowDateTime;
	}

	return seconds;
}

int DataAccumulator::GetElapsedSecondsFromLastLog() 
{
	const int PERIODS_IN_SEC = 10000000; // Number of 100 nanosec periods in second.
	SYSTEMTIME stNow;
	GetLocalTime(&stNow);
	// This will also update mLastLogTime to current time
	int seconds = GetElapsedSecondsFrom(&mLastLogTime, TRUE);

	if (mIsFirstLog) // Is first log operation for today?
	{
		seconds = INT_MAX;
		mFirstLogInFile = stNow;
		mIsFirstLog = FALSE; 
	} else {
		// Same day as we started this log? If no, next log entry will be stored in another file
		if (mFirstLogInFile.wDay != stNow.wDay || mFirstLogInFile.wMonth != stNow.wMonth || mFirstLogInFile.wYear != stNow.wYear) {
			mIsFirstLog = TRUE; 
		}
	}

	return seconds;
}

LPCWSTR DataAccumulator::GetHtmlFromResource(int resourceId, PWORD length)
{
	WCHAR* result = NULL;
	WORD errorCode;
	HMODULE hModule = GetModuleHandle(NULL);
	HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_HTML);

	if (hResource)
	{
		HGLOBAL hLoadedResource = LoadResource(hModule, hResource);

		if (hLoadedResource)
		{
			result = (WCHAR*) LockResource(hLoadedResource);
			*length = SizeofResource(hModule, hResource);
		}
	}

	return result;
};

void DataAccumulator::GenerateLogFileName(PSTR fileName) {
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	strcpy(fileName, DataProvider::DATA_DIR);
	strcat(fileName, "\\");
	char date[20];
	sprintf(date, "%02hu_%02hu_%02hu", localTime.wYear, localTime.wMonth, localTime.wDay);
	strcat(fileName, date);
	strcat(fileName, "\\");
	strcat(fileName, DATA_FILE_NAME);
}

void DataAccumulator::FlushChunk() {
	// Between flush operations the log is retained as valid HTML file!
	const WCHAR* HTML_FOOTER = L"\t\t\t</table>\n\t\t</center>\n\t</body>\n</html>";
	// TODO: Who will close log for previous day???
 	WCHAR footer[512] = {0}; // Yeah! Call it magic number!

	// Build the page footer
	if (mPreviousRowExists) {
		wcscpy(footer, SECTION_END_TAGS);
	}
	wcscat(footer, HTML_FOOTER);

	// Compile full log file name
	CHAR fileName[LOG_FILE_NAME_LENGTH];
	GenerateLogFileName(fileName);
	MakeSureDirectoryPathExists(fileName);

	// TODO: simplify this:
	if (mIsFirstLog) {
		// Write to yesterday's log
		if (mIsLogFileGenerated) { // if not yet generated
			strcpy(mCurrentLogFile, fileName);
		}
	} else {
		// Write to today's log
		strcpy(mCurrentLogFile, fileName);
	}
	mIsLogFileGenerated = TRUE;

	// Create new log file or use existing
	HANDLE logHandle = CreateFileA(mCurrentLogFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD writtenBytes;
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// In case the file exists, seek to where last log entry was written
		// NOTE: Actually we should keep last footer lenght in some field and use it for seeking here.
		//       But since length of footer for second and further flush operations always includes 
		//       length of previous row closing tags (see the if condition above) its OK to use it for
		//       all subsequent seek-in-file operations.
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
