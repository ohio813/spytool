#include "DataAccumulator.h"
#include "DataProvider.h"
#include <Windows.h>
#include <stdio.h>

DataAccumulator::DataAccumulator(void)
{
	mLastLogTime = NULL;
}


DataAccumulator::~DataAccumulator(void)
{
}

void DataAccumulator::LogKey(PWSTR key)
{

}

void DataAccumulator::LogVideo(PTSTR videoFile)
{
}

void DataAccumulator::LogPrintScreen(PTSTR imageFile)
{
}

void DataAccumulator::Log2HTML(PSTR htmlString) {
    CHAR appendee[4096];

    int elapsedSeconds = GetElapsedSecondsFromLastLog();
    if (elapsedSeconds > 60*5) // 5 min.
    {
        // TODO: close previous row!
        PSTR rowHeader = "                <tr>\n                    <td>";
        strcpy(appendee, rowHeader);

        SYSTEMTIME time;
        GetSystemTime(&time);
        char timeS[20];// strlen(12.12.2012 12:12:12\0) == 20
        sscanf(timeS, "%d.%d.%d %d:%d:%d</td>\n", time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute, time.wSecond);
        strcpy(appendee, timeS);

        PSTR openDataTdTag = "                    <td>";
        strcpy(appendee, openDataTdTag);
    }
    else if (elapsedSeconds > 60) // 1 min
    {
        // TODO: close previous paragraph!
        PSTR openParagraphTag = "<p>";
        strcpy(appendee, openParagraphTag);
    }
    else if (elapsedSeconds > 10)
    {
        PSTR newLineBreak = "<br>";
        strcpy(appendee, newLineBreak);
    }

    strcpy(appendee, htmlString);
}


int DataAccumulator::GetElapsedSecondsFromLastLog() 
{
	const int PERIODS_IN_SEC = 10000000; // Number of 100 nanosec periods in second.

	int seconds = 0;

	SYSTEMTIME now;
	PFILETIME nowFT = new FILETIME;
	GetLocalTime(&now);
	SystemTimeToFileTime(&now, nowFT);

	if (mLastLogTime != NULL) 
	{
		seconds = ((__int64) mLastLogTime - (__int64) nowFT) / PERIODS_IN_SEC;
		delete mLastLogTime;
	}

	mLastLogTime = nowFT;

	return seconds;
}