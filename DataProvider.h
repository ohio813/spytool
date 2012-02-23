#pragma once
#include <Windows.h>
#include "DataAccumulator.h"

class DataProvider
{
private:
	bool mIsEnabled;
	DataAccumulator* mDataAccumulator;

protected:
	PSTR GetNewDataFileName();
	virtual PSTR GetExtension() = 0;
	virtual PSTR GetName() = 0;

public:
	static const PSTR const DATA_DIR;

	DataProvider(void)
	{
		mIsEnabled = false;
	}

	~DataProvider(void)
	{
	}

	bool IsEnabled()
	{
		return mIsEnabled;
	}
	
	void SetEnabled(bool enabled)
	{
		mIsEnabled = enabled;
	}

	DataAccumulator* GetDataAccumulator()
	{
		return mDataAccumulator;
	};

	void SetDataAccumulator(DataAccumulator* dataAccumulator)
	{
		mDataAccumulator = dataAccumulator;
	}

	virtual void Init() = 0;
	virtual void Finalize() = 0;
};

