#include "DiskQuotaWatcher.h"
#include <Windows.h>

DiskQuotaWatcher::DiskQuotaWatcher(wchar_t* logsDir)
{
	root = new Directory(logsDir);
	// Build object model that reflects physical directory structure
	TraversePath(root);
}


DiskQuotaWatcher::~DiskQuotaWatcher(void)
{
	delete root;
}

bool DiskQuotaWatcher::TraversePath(Directory* dir)
{ 
	WIN32_FIND_DATAW fdFile; 
	HANDLE hFind = NULL; 

	wchar_t sPath[2048];

	//Specify a file mask. *.* = We want everything! 
	wsprintfW(sPath, L"%s\\*.*", dir->GetName()); 

	if((hFind = FindFirstFileW(sPath, &fdFile)) == INVALID_HANDLE_VALUE) { 
		return false;  // Path is invalid
	} 

	do { 
		//Find first file will always return "." and ".." as the first two directories. 
		if(wcscmp(fdFile.cFileName, L".") != 0 && wcscmp(fdFile.cFileName, L"..") != 0) { 
			//Build up our file path using the passed in 
			//  [dir] and the file/directory we just found: 
			wsprintfW(sPath, L"%s\\%s", dir->GetName(), fdFile.cFileName); 

			//Is the entity a File or Folder? 
			if(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { 
				Directory* newDir = new Directory(sPath);
				dir->AddEntity(newDir);
				TraversePath(newDir); // recursion call 
			} 
			else{
				File* newFile = new File(sPath);
				dir->AddEntity(newFile);
			} 
		}
	} 
	while(FindNextFileW(hFind, &fdFile)); //Find the next file. 

	FindClose(hFind); //Always, Always, clean things up! 

	return true; 
} 