#pragma once

#include "part.h"
#include "fs.h"
#include "KernelFile.h"
#include "ReadWrite.h"
#include <map>
#include <string>

class WriteLock;

class RootDirectory :public KernelFile {
public:
	RootDirectory(BitVector * bitVector);
	~RootDirectory();
	BytesCnt ReadEntry(Entry* entry, EntryNum entryNum);
	KernelFile* OpenFile(char* fileName, char mode);
	bool DeleteFile(char* fileName);
	bool DoesExist(char* fileName);
	void Format();
	void WaitToCloseAllFiles();
	ReadWrite * GetSyncForFile(char * name);
private:
	void AddFileToRegistry(KernelFile * file);
	void CloseFile(KernelFile * file);
	void ClearOpenFiles();
	void SynchronizeFile(KernelFile * file);

	void LoadRoot();
	bool Compare(char * name1, char * name2);
	bool Compare(Entry entry1, Entry entry2);

	KernelFile* CreateFile(char* fileName, char mode);

	BytesCnt WriteEntry(Entry* entry, EntryNum entryNum);
	BytesCnt GetEntryPositionInFile(EntryNum entryNum);

	KernelFile* FindFile(const char* name);
	bool DeleteEntry(EntryNum entryNum);

	BytesCnt Write(BytesCnt num, char * buffer);

	EntryNum FindFileEntryNum(const char * name);

	int equals(Entry* e, const char* fileName);
	char* fname(Entry * e);

	void IsLoad();

	void MakeEmptyEntry(Entry * entry);

	bool m_IsLoad;

	EntryNum m_NoFile;
	BitVector * m_bitVector;

	std::map<std::string, ReadWrite*> m_openFile;
	std::map<std::string, int> m_fileCount;

	ReadWrite m_waitToCloseAllFiles;
	WriteLock* m_lockWaitToCloseAllFiles;
	friend class KernelFile;
	friend class KernelFS;
};