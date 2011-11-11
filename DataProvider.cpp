#pragma once
#include "DataProvider.h"
#include <stdio.h>
#include <Windows.h>

const PSTR const DATA_DIR = "accumulated";

// Returns DATA_DIR + / + date + / + DataProvider.Name + / + time + . + ext
PSTR DataProvider::GetNewDataFileName()
{
	PSTR fileName = new CHAR[100];
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	
	strcpy(fileName, DATA_DIR);
	strcpy(fileName, "\\");

	char date[20];
	sscanf(date, "%d_%d_%d", localTime.wDay, localTime.wMonth, localTime.wYear);
	strcpy(fileName, date);

	strcpy(fileName, "\\");
	strcpy(fileName, GetName());
	strcpy(fileName, "\\");

	char time[20];
	sscanf(time, "%d_%d_%d", localTime.wHour, localTime.wMinute, localTime.wSecond);
	strcpy(fileName, time);
	
	strcpy(fileName, ".");
	strcpy(fileName, GetExtension());

	return fileName;
}