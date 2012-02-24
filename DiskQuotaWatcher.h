#pragma once
#include "FileSystemEntity.h"

class DiskQuotaWatcher
{
private:
	bool TraversePath(Directory* dir);
	Directory* root;
	int quota; // in bytes

public:
	static const int DEFAULT_QUOTA = 10*1024*1024; // 10 MB

	DiskQuotaWatcher(PSTR logsDir, int quota);
	~DiskQuotaWatcher(void);
	int GetQuota() { return quota; }
	void SetQuota(int quota) { this->quota = quota; }
	void KeepLogsWithinQuota();
	void RegisterNewFile(PSTR file);
};

