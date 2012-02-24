#pragma 
#include "FileSystemEntity.h"

class DiskQuotaWatcher
{
private:
	bool TraversePath(Directory* dir);
	Directory* root;
	int quota; // in bytes

public:
	DiskQuotaWatcher(PSTR logsDir, int quota);
	~DiskQuotaWatcher(void);
	int GetQuota() { return quota; }
	void SetQuota(int quota) { this->quota = quota; }
	void KeepLogsWithinQuota();
	void RegisterNewFile(PSTR file);
};

