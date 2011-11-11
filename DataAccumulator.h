#pragma once
#include <Windows.h>

class DataAccumulator
{
private:
	// Specifies maximum disk space to use by the program
	int mDiskQuota;
public:
	DataAccumulator(void);
	~DataAccumulator(void);
	void SetDiskQuota(int quota) { mDiskQuota = quota;}
};

