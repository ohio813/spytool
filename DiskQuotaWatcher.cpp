#include "DiskQuotaWatcher.h"
#include <Windows.h>

DiskQuotaWatcher::DiskQuotaWatcher(wchar_t* logsDir, int quota)
	:quota(quota) // field init
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

	wchar_t* sPath = new wchar_t[2048];

	//Specify a file mask. *.* = We want everything! 
	wsprintfW(sPath, L"%s\\*.*", dir->GetName()); 

	if((hFind = FindFirstFileW(sPath, &fdFile)) == INVALID_HANDLE_VALUE) { 
		return false;  // Path is invalid
	} 

	do { 
		//Find first file will always return "." and ".." as the first two directories. 
		if(wcscmp(fdFile.cFileName, L".") != 0 && wcscmp(fdFile.cFileName, L"..") != 0) { 
			// Build up our file path using the passed in 
			// [dir] and the file/directory we just found: 
			wsprintfW(sPath, L"%s\\%s", dir->GetName(), fdFile.cFileName); 

			//Is the entity a File or a Folder? 
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
	while(FindNextFileW(hFind, &fdFile)); //Find next file. 

	FindClose(hFind); // free handle 

	return true; 
} 

void DiskQuotaWatcher::KeepLogsWithinQuota() {
	while (root->GetSize() > quota) {
		root->DeleteOldestEntity();
	}
}

void DiskQuotaWatcher::RegisterNewFile(wchar_t* file) {
	// Preserve full file name
	wchar_t* fileName = new wchar_t[wcslen(file) + 1]; // the memory is deallocated in destructor 
	wcscpy(fileName, file);

	wchar_t* dirName = wcstok(file, L"/\"");
	Directory* current = root;
	FileSystemEntity* foundEntity;

	while (dirName != NULL) {
		foundEntity = current->FindEntity(dirName);
		if (foundEntity != NULL) {
			Directory* foundEntityAsDir = dynamic_cast<Directory*>(foundEntity);
			if (foundEntityAsDir != NULL) { // is it dir?
				current = foundEntityAsDir;
			} else { 
				return; // The file is already existing
			}
		} else {
			break; // current is directory where a new file should be added.
		}
		
		// Get next directiry from path
		dirName = wcstok(NULL, L"/\"");
	}

	// TODO: Check if file must have full path in its name
	current->AddEntity(new File(fileName)); 
}
