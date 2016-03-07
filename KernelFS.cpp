#include "KernelFS.h"
#include "MountedPartition.h"
#include "RootDirectory.h"
#include "file.h"
#include "ReadLock.h"
#include "ReadWrite.h"
#include "WriteLock.h"

const int c_num = 26;

KernelFS::~KernelFS()
{
	WriteLock lock(*m_sync);
	Delete();
}

void KernelFS::Delete()
{
	for (int i = 0; i < 26; i++)
	{
		if (m_mountPartisions[i] != 0)
		{
			delete m_mountPartisions[i];
			m_mountPartisions[i] = 0;
		}
	}
}

char KernelFS::Mount(Partition* partition)
{
	WriteLock lock(*m_sync);
	int i = 0;
	for (i; i < c_num; i++)
	{
		if (m_mountPartisions[i] == 0)
		{
			m_mountPartisions[i] = new MountedPartition();
			m_mountPartisions[i]->Mount(partition);
			return 'A' + i;
		}
	}
	return 0;
}

char KernelFS::Unmount(char part)
{
	WriteLock lock(*m_sync);
	int index = part - 'A';
	if (m_mountPartisions[index] == 0)
	{
		return 0;
	}
	WriteLock lockPartisions(*m_mountPartisions[index]->GetSync());
	m_mountPartisions[index]->Unmount();
	delete m_mountPartisions[index];
	m_mountPartisions[index] = 0;
	return 1;
}

char KernelFS::Format(char part)
{
	int index = part - 'A';
	if (m_mountPartisions[index] != 0)
	{
		WriteLock lock(*m_mountPartisions[index]->GetSync());
		m_mountPartisions[index]->Format();
		return true;
	}
	return false;
}

char KernelFS::DoesExist(char* fname)
{
	ReadLock lock(*m_sync);
	char part = fname[0];
	if (m_mountPartisions[part - 'A'] != 0)
	{
		ReadLock lockPartisions(*m_mountPartisions[part - 'A']->GetSync());
		RootDirectory* rootDir = m_mountPartisions[part - 'A']->GetRoot();
		ReadLock lockRoot(*rootDir->GetSync());
		return rootDir->DoesExist(fname);
	}
	return false;
}



File* KernelFS::Open(char* fname, char mode)
{
	bool addToOpen = false;
	bool upDate = false;
	char partitionName = fname[0];
	int index = partitionName - 'A';
	if (m_mountPartisions[index] != 0)
	{
		KernelFile* file = nullptr;
		ReadLock lockPartisions(*m_mountPartisions[index]->GetSync());
		RootDirectory* rootDir = m_mountPartisions[index]->GetRoot();
		{
			bool exist;
			{
				WriteLock lockRoot(*rootDir->GetSync());
				exist = rootDir->DoesExist(fname);
			}
			if (!((mode == 'r' || mode == 'a') && exist == false))
			{
				ReadWrite * sync = nullptr;
				{
					WriteLock lockRoot(*rootDir->GetSync());
					sync = rootDir->GetSyncForFile(fname);
				}
				if (sync != nullptr)
				{
					ReadLock fileReadLock(*sync);
					WriteLock lockRoot(*rootDir->GetSync());
					file = rootDir->OpenFile(fname, mode);
				}
				else
				{
					WriteLock lockRoot(*rootDir->GetSync());
					file = rootDir->OpenFile(fname, mode);
				}
			}
			else
			{
				return nullptr;
			}
		}

		if (mode == 'w')
		{
			file->LockFileForWrite();
			file->Seek(0);
			file->Truncate();
			upDate = true;
		}
		if (mode == 'a')
		{
			file->LockFileForWrite();
			file->SeekEnd();
		}
		if (mode == 'r')
		{
			file->LockFileForRead();
		}

		File* newFile = new File();
		newFile->myImpl = file;

		if (upDate)
		{
			WriteLock lockRoot(*rootDir->GetSync());
			file->Update();
		}
		return newFile;
	}

	return nullptr;
}

char KernelFS::DeleteFile(char* fname)
{
	bool succeed = false;
	char part = fname[0];
	if (m_mountPartisions[part - 'A'] != 0)
	{
		ReadLock lockPartisions(*m_mountPartisions[part - 'A']->GetSync());
		RootDirectory* rootDir = m_mountPartisions[part - 'A']->GetRoot();
		succeed = rootDir->DeleteFile(fname);
	}
	return succeed;
}

char KernelFS::readRootDir(char part, EntryNum n, Directory & d)
{
	ReadLock lock(*m_sync);
	if (m_mountPartisions[part - 'A'] != 0)
	{
		ReadLock lock(*m_mountPartisions[part - 'A']->GetSync());
		RootDirectory* rootDir = m_mountPartisions[part - 'A']->GetRoot();
		int i;
		for (i = 0; i < 65; i++)
		{
			ReadLock lock(*rootDir->GetSync());
			rootDir->IsLoad();
			BytesCnt readBytesCnt = rootDir->ReadEntry(&d[i], i + n);
			Entry emptyEntry;
			rootDir->MakeEmptyEntry(&emptyEntry);
			if (readBytesCnt == 0)
			{
				break;
			}
		}
		return i;
	}

	return 0;
}

KernelFS::KernelFS()
{
	m_sync = new ReadWrite();
	for (int i = 0; i < c_num; i++)
	{
		m_mountPartisions[i] = 0;
	}
}

