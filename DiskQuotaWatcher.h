#pragma 
#include "FileSystemEntity.h"

class DiskQuotaWatcher
{
private:
	bool TraversePath(Directory* dir);
	Directory* root;
	int quota; // in bytes

public:
	DiskQuotaWatcher(wchar_t* logsDir, int quota);
	~DiskQuotaWatcher(void);
	int GetQuota() { return quota; }
	void SetLogsMaxSize(int logsMaxSize) { this->quota = quota; }
	void KeepLogsWithinQuota();
	void RegisterNewFile(wchar_t* file);
};

