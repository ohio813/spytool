#pragma 
#include "FileSystemEntity.h"

class DiskQuotaWatcher
{
private:
	bool TraversePath(Directory* dir);
	Directory* root;

public:
	DiskQuotaWatcher(wchar_t* logsDir);
	~DiskQuotaWatcher(void);
};

