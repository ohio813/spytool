#pragma once
#include "DataProvider.h"
#include <stdio.h>
#include <Windows.h>

const PSTR const DataProvider::DATA_DIR = "accumulated";

// Returns DATA_DIR + / + date + / + DataProvider.Name + / + time + . + ext
PSTR DataProvider::GetNewDataFileName()
{
	PSTR fileName = new CHAR[100];
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	
	strcpy(fileName, DATA_DIR);
	strcat(fileName, "\\");

	char date[20];
	sprintf(date, "%02hu_%02hu_%02hu", localTime.wYear, localTime.wMonth, localTime.wDay);
	strcat(fileName, date);

	strcat(fileName, "\\");
	strcat(fileName, GetName());
	strcat(fileName, "\\");

	char time[20];
	sprintf(time, "%02hu_%02hu_%02hu", localTime.wHour, localTime.wMinute, localTime.wSecond);
	strcat(fileName, time);
	
	strcat(fileName, ".");
	strcat(fileName, GetExtension());

	return fileName;
}