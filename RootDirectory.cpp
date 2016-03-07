#include "RootDirectory.h"
#include "KernelFile.h"
#include "KernelFsExceptions.h"
#include <cstring>
#include "IClusterManager.h"
#include "WriteLock.h"
#include "ReadLock.h"


static const unsigned c_entrySize = 20;

RootDirectory::RootDirectory(BitVector * bitVector)
	: KernelFile(nullptr, bitVector, 1, 0, "", 'a')
	, m_bitVector(bitVector)
	, m_NoFile(0)
	, m_lockWaitToCloseAllFiles(nullptr)
{
	m_sync = new ReadWrite();
}

RootDirectory::~RootDirectory()
{
	WriteLock lock(*m_sync);
	delete m_bitVector;
	m_bitVector = nullptr;
	delete m_lockWaitToCloseAllFiles;
	m_lockWaitToCloseAllFiles = nullptr;
}

BytesCnt RootDirectory::WriteEntry(Entry* entry, EntryNum entryNum)
{
	BytesCnt writeBytes = 0;
	BytesCnt entryPositonInFile = GetEntryPositionInFile(entryNum);
	m_file_position = entryPositonInFile;

	if (m_file_size == 0 && m_file_position == 0)
	{
		m_memManager->AddNewCluster();
	}

	while (!Seek(m_file_position)) // Dohvati data cluster.
	{
		m_file_size = m_file_position + 1;
	}
	if (Seek(entryPositonInFile))
	{
		writeBytes += Write(FNAMELEN, entry->name);
		writeBytes += Write(FEXTLEN, entry->ext);

		char* reservedBuffer = (char*)&entry->reserved;
		writeBytes += Write(1, reservedBuffer);

		char* firstClusterBuffer = (char*)&entry->indexCluster;
		writeBytes += Write(4, firstClusterBuffer);

		char* sizeBuffer = (char*)&entry->size;
		writeBytes += Write(4, sizeBuffer);
	}
	return writeBytes;
}
BytesCnt RootDirectory::ReadEntry(Entry* entry, EntryNum entryNum)
{
	BytesCnt readBytes = 0;
	BytesCnt entryPositonInFile = GetEntryPositionInFile(entryNum);

	if (Seek(entryPositonInFile))
	{
		readBytes += Read(FNAMELEN, entry->name);
		readBytes += Read(FEXTLEN, entry->ext);

		char* reservedBuffer = (char*)&entry->reserved;
		readBytes += Read(1, reservedBuffer);

		char* firstClusterBuffer = (char*)&entry->indexCluster;
		readBytes += Read(4, firstClusterBuffer);

		char* sizeBuffer = (char*)&entry->size;
		readBytes += Read(4, sizeBuffer);
	}
	return readBytes;
}

BytesCnt RootDirectory::GetEntryPositionInFile(EntryNum entryNum)
{
	return entryNum * c_entrySize;
}


bool RootDirectory::DeleteEntry(EntryNum entryNum)
{
	Entry lastEntry;
	ReadEntry(&lastEntry, m_NoFile - 1);
	WriteEntry(&lastEntry, entryNum);
	char empty[c_entrySize] = { 0 };
	WriteEntry((Entry*)&empty, m_NoFile - 1);
	Seek(GetEntryPositionInFile(m_NoFile - 1));
	Truncate();

	m_NoFile--;
	return true;
}

KernelFile* RootDirectory::CreateFile(char* fileName, char mode)
{
	KernelFile* file = new KernelFile(this, m_bitVector, fileName, mode);

	Entry entry = file->FromFileToFCB();
	if (c_entrySize == WriteEntry(&entry, m_NoFile))
	{
		{
			m_NoFile++;
			return file;
		}
	}
	return nullptr;
}

KernelFile * RootDirectory::OpenFile(char * fileName, char mode)
{
	IsLoad();
	KernelFile * file = FindFile(fileName);

	if (file != nullptr)
	{
		file->SetMode(mode);
	}
	else
	{
		file = CreateFile(fileName, mode);
	}
	AddFileToRegistry(file);
	SynchronizeFile(file);
	return file;
}

void RootDirectory::AddFileToRegistry(KernelFile * file)
{
	std::string path(file->m_name);
	auto pom = m_fileCount.find(path);
	if (pom != m_fileCount.end())
	{
		m_fileCount[path]++;
	}
	else
	{
		m_fileCount[path] = 1;
	}
}

void RootDirectory::CloseFile(KernelFile * file)
{
	std::string path(file->m_name);
	auto pom = m_fileCount.find(path);
	if (pom != m_fileCount.end())
	{
		m_fileCount[path]--;
	}
	if (0 == m_fileCount[path])
	{
		auto openFile = m_openFile.find(path);
		if (openFile != m_openFile.end())
		{
			bool free = false;
			for (auto it = m_fileCount.begin(); it != m_fileCount.end(); ++it)
			{
				free = (it->second == 0);
				if (!free)
				{
					break;
				}
			}
			if (free)
			{
				if (m_lockWaitToCloseAllFiles != nullptr)
				{
					delete m_lockWaitToCloseAllFiles;
					m_lockWaitToCloseAllFiles = nullptr;
				}
			}
		}
	}
}

void RootDirectory::ClearOpenFiles()
{
	for (auto openFile = m_openFile.begin(); openFile != m_openFile.end(); openFile++)
	{
		delete openFile->second;
		openFile->second = nullptr;
	}
	m_openFile.clear();
	m_fileCount.clear();
	if (m_lockWaitToCloseAllFiles != nullptr)
	{
		delete m_lockWaitToCloseAllFiles;
		m_lockWaitToCloseAllFiles = nullptr;
	}
}

bool RootDirectory::DeleteFile(char * fileName)
{
	IsLoad();
	KernelFile* file = FindFile(fileName);
	if (file != nullptr)
	{
		std::string path(file->GetName());
		auto pom = m_openFile.find(path);
		if (pom != m_openFile.end())
		{
			EntryNum entryNum = FindFileEntryNum(fileName);
			file->Seek(0);
			file->Truncate();
			ClusterNo firstCluster = file->m_memManager->GetAddressOfFirstCluster();
			m_bitVector->SetIsFree(firstCluster, true);
			DeleteEntry(entryNum);
			return true;
		}
	}
	return false;
}

bool RootDirectory::DoesExist(char * fileName)
{
	IsLoad();
	EntryNum file = FindFileEntryNum(fileName);
	if (file != -1)
	{
		return true;
	}
	return false;
}

void RootDirectory::Format()
{
	m_bitVector->Format();
	Cluster cluster;
	cluster.Load(m_partition, 1);
	cluster.Clear();
	delete m_memManager;
	m_memManager = m_bitVector->GetClusterManager(1);
	m_file_size = 0;
	m_file_position = 0;
	m_NoFile = 0;
}

void RootDirectory::WaitToCloseAllFiles()
{
	WriteLock lock(m_waitToCloseAllFiles);
}

ReadWrite * RootDirectory::GetSyncForFile(char * name)
{
	std::string path(name);
	auto pom = m_openFile.find(path);
	if (pom != m_openFile.end())
	{
		return pom->second;
	}
	else
	{
		return nullptr;
	}

}

BytesCnt RootDirectory::Write(BytesCnt num, char * buffer) {
	{
		BytesCnt leftToWrite = num;
		char* writeBuffer = buffer;
		while (leftToWrite > 0 && !m_bitVector->EndOfCluster())
		{
			bool needToClear = false;
			if (m_file_position == GetCapacity())
			{
				// Prosiri file.
				Verify(m_bitVector->EndOfCluster());
				m_memManager->AddNewCluster();
				LoadCluster(m_file_position);
				needToClear = true;
			}
			while (!Seek(m_file_position)) // Dohvati data cluster.
			{
				m_file_size = m_file_position + 1;
			}
			if (needToClear)
			{
				m_data.Clear();
			}
			const BytesCnt writeBytesFromCurrentDataCluster =
				m_data.Write(writeBuffer, leftToWrite);

			Verify(leftToWrite >= writeBytesFromCurrentDataCluster);

			leftToWrite -= writeBytesFromCurrentDataCluster;
			m_file_position += writeBytesFromCurrentDataCluster;
			writeBuffer += writeBytesFromCurrentDataCluster;
		}
		if (m_file_position >= m_file_size)
		{
			m_file_size = m_file_position;
		}
		return num - leftToWrite;
	}
}

EntryNum RootDirectory::FindFileEntryNum(const char* name)
{
	EntryNum entryNo = 0;
	Entry file;
	while (c_entrySize == ReadEntry(&file, entryNo) && entryNo < m_NoFile)
	{
		if (0 == equals(&file, name))
		{
			return entryNo;
		}
		entryNo = entryNo + 1;
	}
	return -1;
}

KernelFile* RootDirectory::FindFile(const char* name)
{
	EntryNum entryNo = 0;
	Entry file;
	while (c_entrySize == ReadEntry(&file, entryNo) && entryNo < m_NoFile)
	{
		if (0 == equals(&file, name))
		{
			return new KernelFile(this, m_bitVector, file, 'a');
		}
		entryNo = entryNo + 1;
	}
	return nullptr;
}

int RootDirectory::equals(Entry* e, const char* fileName) {
	char * name = fname(e);
	int res = strcmp(name, fileName);
	delete[] name;
	return res;
}

char * RootDirectory::fname(Entry * e) {
	char * name = new char[13];
	int i = 0;
	if (e->name[0] != 0) {
		for (i = 0; i < 8; i++) {
			if (e->name[i] == ' ') break;
			name[i] = e->name[i];
		}
	}
	name[i++] = '.';
	if (e->ext[0] != 0) {
		int k = i;
		for (int j = 0; j < 3; j++) {
			if (e->ext[j] == ' ')break;
			name[i++] = e->ext[j];
		}
	}
	name[i] = '\0';
	return name;
}

void RootDirectory::IsLoad()
{
	if (!m_IsLoad)
	{
		LoadRoot();
	}
}

void RootDirectory::MakeEmptyEntry(Entry * entry) {
	char name[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '\0' };
	char ext[] = { 0x00, 0x00, '\0' };
	entry->reserved = 0;
	entry->indexCluster = 0;
	entry->size = 0;
	for (int i = 0; i < FNAMELEN; i++)
	{
		entry->name[i] = '\0';
	}
	for (int i = 0; i < FEXTLEN; i++)
	{
		entry->ext[i] = '\0';
	}
}

void RootDirectory::SynchronizeFile(KernelFile * file)
{
	if (file != nullptr)
	{
		if (m_openFile.empty())
		{
			m_lockWaitToCloseAllFiles = new WriteLock(m_waitToCloseAllFiles);
		}
		std::string path(file->GetName());
		auto pom = m_openFile.find(path);
		if (pom != m_openFile.end())
		{
			file->SetSync(pom->second);
		}
		else
		{
			m_openFile[path] = new ReadWrite();
			auto pom = m_openFile.find(path);
			if (pom != m_openFile.end())
			{
				file->SetSync(pom->second);
			}
		}
	}
}

void RootDirectory::LoadRoot()
{
	EntryNum entryNum = 0;
	Entry entry;
	Entry emptyEntry;
	MakeEmptyEntry(&entry);
	MakeEmptyEntry(&emptyEntry);

	m_memManager->Load(1);
	m_file_size = GetCapacity(); // PREPOSTAVKA DA JE SKROZ PUN JER NECE DA MI 
								 // CITA READENTRY JER JE SIZE = 0 PA NE MOZE DA SE POZICIONIRA

	ReadEntry(&entry, entryNum);
	while (!Compare(emptyEntry, entry))
	{
		m_NoFile++;
		entryNum++;
		ReadEntry(&entry, entryNum);
	}
	m_file_size = m_NoFile * c_entrySize;
	m_IsLoad = true;
}

bool RootDirectory::Compare(char* name1, char* name2) {
	std::string name1str(name1);
	std::string name2str(name2);
	return name1str.compare(name2str) == 0;
}

bool RootDirectory::Compare(Entry entry1, Entry entry2) {
	return (entry1.size == entry2.size
		&& entry1.indexCluster == entry2.indexCluster
		&& entry1.reserved == entry2.reserved
		&& Compare(entry1.name, entry2.name)
		&& Compare(entry1.ext, entry2.ext));
}