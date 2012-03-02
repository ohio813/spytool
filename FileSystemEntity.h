#pragma once
#include "ListQueue.h"
#include <Windows.h>
#include <Shlwapi.h>

// Fuck the CPP .H and .CPP files 
// WTF!??!? Why duplicate the code? Crappy language.


class FileSystemEntity {
private:
	PSTR name;
	int size;

protected: 
	virtual void CalcSize() = 0;

	void SetSize(int size) {
		this->size = size;
	}

public:
	FileSystemEntity() {
		name = NULL;
		size = 0;
	}

	FileSystemEntity(PSTR name) {
		this->name = new CHAR[strlen(name) + 1];
		strcpy(this->name, name);
		size = 0;
	}

	~FileSystemEntity() {
		if (name != NULL) {
			delete name;
		}
	}

	void SetName(PSTR name) {
		this->name = new CHAR[strlen(name) + 1];
		strcpy(this->name, name);
	}

	PSTR GetName() {
		return name;
	}

	int GetSize() {
		if (size == 0) {
			CalcSize();
		}
		return size;
	}

	virtual void Delete() = 0;
	virtual PSTR ListContents(int level) = 0;
};

class File : public FileSystemEntity {
protected:
	void CalcSize() {
		HANDLE hFile = CreateFile (GetName(), 0, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
		DWORD lowSize;
		DWORD highSize;

		if (hFile != INVALID_HANDLE_VALUE) {
			lowSize = GetFileSize (hFile, &highSize);
			if (lowSize == INVALID_FILE_SIZE) {
				DWORD errorCode = GetLastError();
				if (errorCode != ERROR_SUCCESS) {
					lowSize = highSize = 0;
				}
			}
			CloseHandle (hFile);
		}

		// We actually ignore high size because we assume our files to be up to 4GB (0xFFFFFFFF)
		SetSize(lowSize);
	}

public: 
	File(PSTR name) : FileSystemEntity(name) {
	}

	void Delete() {
		DeleteFile(GetName());
	}

	PSTR ListContents(int level) {
		return "";
	}
};



class EntityNode: public ListNode {
private:
	FileSystemEntity* fsEntity;

public:
	EntityNode(FileSystemEntity* entity) : ListNode() {
		fsEntity = entity;
	}

	~EntityNode() {
		delete fsEntity;
	}

	FileSystemEntity* GetEntity() {
		return fsEntity;
	}

	void SetEntity(FileSystemEntity* entity) {
		this->fsEntity = entity;
	}
};

class Directory : public FileSystemEntity {
private:
	ListQueue* innerEntities;

protected:
	void CalcSize() {
		int size = 0;
		innerEntities->ResetIter();
		for(int iter = 0; iter < innerEntities->GetCount(); iter++) {
			size += ((EntityNode*)innerEntities->GetNext())->GetEntity()->GetSize();
		}

		SetSize(size);
	}

public:
	Directory(PSTR name) : FileSystemEntity(name) {
		innerEntities = new ListQueue();
	}

	~Directory() {
	}

	void AddEntity(FileSystemEntity* entity) {
		innerEntities->Enqueue(new EntityNode(entity));
	}

	void Delete() {
		innerEntities->ResetIter();
		for(int iter = 0; iter < innerEntities->GetCount(); iter++) {
			((EntityNode*)innerEntities->GetNext())->GetEntity()->Delete();
		}

		RemoveDirectory(GetName());
	}

	void DeleteOldestEntity() {
		if (innerEntities->GetCount() == 0) return; // Nothing to delete

		innerEntities->ResetIter();
		EntityNode *prevOldest = NULL, *oldest = ((EntityNode*)innerEntities->GetNext()); // first entity
		EntityNode *prevNode = NULL, *next = NULL;

		for(int iter = 1; iter < innerEntities->GetCount(); iter++) {
			prevNode = next;
			next = ((EntityNode*)innerEntities->GetNext());
			if (strcmp(oldest->GetEntity()->GetName(), next->GetEntity()->GetName()) > 0) {
				oldest = next;
				prevOldest = prevNode;
			}
		}

		oldest->GetEntity()->Delete(); // Delete file/directory
		innerEntities->Remove(prevOldest); 
	}

	FileSystemEntity* FindEntity(PSTR name) {
		int entitiesCount = innerEntities->GetCount();
		EntityNode* current;

		innerEntities->ResetIter();
		for (int i= 0; i<entitiesCount; i++) {
			current = (EntityNode*)innerEntities->GetNext();

			if (strcmp(current->GetEntity()->GetName(), name) == 0) {
				return current->GetEntity();
			}
		}

		return NULL;
	}

	PSTR ListContents() {
		return ListContents(0);
	}

	// level should be between 0 and 5
	PSTR ListContents(int level) {
		PSTR dirTree = new CHAR[4096];
		memset(dirTree, 0, 4096);
		PCHAR tabs = "\t\t\t\t\t";

		int entitiesCount = innerEntities->GetCount();
		EntityNode* current;

		innerEntities->ResetIter();
		for (int i= 0; i<entitiesCount; i++) {
			current = (EntityNode*)innerEntities->GetNext();

			strcat(dirTree, tabs + (5 - level));
			strcat(dirTree, current->GetEntity()->GetName());
			strcat(dirTree, "\n");
			PSTR innerContents = current->GetEntity()->ListContents(level + 1);
			strcat(dirTree, innerContents);
		}

		return dirTree;
	}
};
