//#include <string>
#pragma warning(disable:4996)
#include <cstring>
#include "KernelFile.h"
#include "IClusterManager.h"
#include "KernelFsExceptions.h"
#include "RootDirectory.h"
#include "WriteLock.h"
#include "ReadLock.h"

static const unsigned c_entrySize = 4;
static const EntryNum c_entriesPerCluster = ClusterSize / c_entrySize;
static const EntryNum c_directEntries = c_entriesPerCluster / 2;

KernelFile::KernelFile(RootDirectory * root, BitVector * bitVector, ClusterNo addressOfFirstCluster
	, BytesCnt fileSize, char* name, char mode)
	: m_file_size(fileSize)
	, m_file_position(0)
	, m_mode(mode)
	, m_root(root)
	, m_sync(nullptr)
	, m_readLock(nullptr)
	, m_writeLock(nullptr)
{
	m_memManager = bitVector->GetClusterManager(addressOfFirstCluster);
	m_partition = bitVector->GetPartition();
	SetName(name);
}

KernelFile::KernelFile(RootDirectory * root, BitVector * bitVector, char* name, char mode)
	: m_file_size(0)
	, m_file_position(0)
	, m_mode(mode)
	, m_root(root)
	, m_sync(nullptr)
	, m_readLock(nullptr)
	, m_writeLock(nullptr)
{
	m_memManager = bitVector->GetClusterManager();
	m_partition = bitVector->GetPartition();
	SetName(name);
}

KernelFile::KernelFile(RootDirectory * root, BitVector * bitVector, Entry fcb, char mode)
	: m_file_position(0)
	, m_file_size(fcb.size)
	, m_mode(mode)
	, m_root(root)
	, m_sync(nullptr)
	, m_readLock(nullptr)
	, m_writeLock(nullptr)
{
	m_memManager = bitVector->GetClusterManager(fcb.indexCluster);
	m_partition = bitVector->GetPartition();
	const char* name = GetNameFromFCB(&fcb);
	SetName(name);
}

BytesCnt KernelFile::FileSizeWithSync()const
{
	return m_file_size;
}

BytesCnt KernelFile::FileSize()const
{
	return m_file_size;
}

bool KernelFile::IsEof() const
{
	return m_file_position >= m_file_size;
}

bool KernelFile::Seek(BytesCnt pos)
{
	if (pos >= m_file_size)
	{
		return false;
	}
	m_file_position = pos;
	LoadCluster(pos);
	return true;
}
void KernelFile::SeekEnd()
{
	m_file_position = m_file_size;
	if (m_file_size == 0)
	{
		m_memManager->AddNewCluster();
	}
	LoadCluster(m_file_position);
}
void KernelFile::LoadCluster(BytesCnt position)
{
	int clusterIndex = GetClusterIndex(position);
	BytesCnt positionIncluster = GetPoistionInCluster(position);
	Verify(positionIncluster >= 0 && positionIncluster < ClusterSize);

	m_memManager->GetCluster(clusterIndex, m_data);

	m_data.Seek(positionIncluster);
}

ClusterNo KernelFile::GetClusterIndex(BytesCnt position)
{
	return position / ClusterSize;
}

BytesCnt KernelFile::GetPoistionInCluster(BytesCnt position)
{
	return position % ClusterSize;
}

BytesCnt KernelFile::Read(BytesCnt num, char* buffer)
{
	if (IsEof())
	{
		return 0;
	}

	BytesCnt 	leftToRead = num;
	char* 	readBuffer = buffer;
	while (leftToRead > 0 && !IsEof())
	{
		Seek(m_file_position); // Dohvati data cluster.
		BytesCnt bytesToRead = leftToRead;
		if (m_file_position + leftToRead > m_file_size && m_file_position + ClusterSize > m_file_size)
		{
			bytesToRead = m_file_size - m_file_position;
		}
		const BytesCnt readBytesFromCurrentDataCluster =
			m_data.Read(readBuffer, bytesToRead);

		Verify(leftToRead >= readBytesFromCurrentDataCluster);

		leftToRead -= readBytesFromCurrentDataCluster;
		m_file_position += readBytesFromCurrentDataCluster;
		readBuffer += readBytesFromCurrentDataCluster;
	}
	return num - leftToRead;
}

BytesCnt KernelFile::Write(BytesCnt num, char * buffer)
{

	BytesCnt leftToWrite = num;
	char* writeBuffer = buffer;
	while (leftToWrite > 0) //&& !m_bitVector.EndOfCluster())
	{
		bool needsNewCluster = (m_file_position == GetCapacity());
		if (needsNewCluster) {
			// Prosiri file.
			m_memManager->AddNewCluster();
			LoadCluster(m_file_position);
		}
		while (!Seek(m_file_position)) // Dohvati data cluster.
		{
			m_file_size = m_file_position + 1;
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

BytesCnt KernelFile::GetCapacity() const
{
	return m_memManager->GetClusterCount() * ClusterSize;
}

bool KernelFile::Delete(BytesCnt position)
{
	if (position >= m_file_size)
	{
		return false;
	}
	int current_index = GetClusterIndex(position);
	if (current_index + 1 < m_memManager->GetClusterCount())
	{
		m_memManager->ReleaseClusters(current_index + 1);
	}

	m_file_size = position;
	m_file_position = position;
	if (m_file_size == 0)
	{
		m_memManager->ReleaseClusters(current_index);
	}
	return true;
}

KernelFile::~KernelFile()
{

	if (m_root != nullptr)
	{
		WriteLock lock(*m_root->GetSync());
		m_root->CloseFile(this);
		if (m_mode != 'r')
		{
			Update();
		}
	}

	m_root = nullptr;
	delete m_memManager;
	m_memManager = nullptr;

	if (m_readLock != nullptr)
	{
		delete m_readLock;
		m_readLock = nullptr;
	}
	if (m_writeLock != nullptr)
	{
		delete m_writeLock;
		m_writeLock = nullptr;
	}

}

BytesCnt KernelFile::GetPos() const
{
	return m_file_position;
}

bool KernelFile::Truncate()
{
	return Delete(m_file_position);
}

void KernelFile::Update()
{
	if (m_root != nullptr)
	{
		Entry entry;
		const char* fileName;
		entry = FromFileToFCB();
		fileName = GetName();
		EntryNum entryNum = m_root->FindFileEntryNum(fileName);
		m_root->WriteEntry(&entry, entryNum);
	}
}

ReadWrite * KernelFile::GetSync()
{
	return m_sync;
}

void KernelFile::LockFileForRead()
{
	m_readLock = new ReadLock(*m_sync);
}

void KernelFile::LockFileForWrite()
{
	m_writeLock = new WriteLock(*m_sync);
}

void KernelFile::SetName(const char* name)
{
	m_name = std::string(name);
}

const std::string KernelFile::GetExt() const
{
	char ext[FEXTLEN + 1] = { '\0' };
	int startExt = m_name.find('.');
	int endName = m_name.length();
	m_name.copy(ext, FEXTLEN, startExt + 1);
	std::string file_ext(ext);
	return file_ext.c_str();
}

const char* KernelFile::GetName() const
{
	return m_name.c_str();
}

const std::string KernelFile::GetFileName() const
{
	char name[FNAMELEN + 1] = { '\0' };
	int endName = m_name.find('.');
	m_name.copy(name, endName, 0);
	std::string file_name(name);
	return file_name.c_str();
}

Entry KernelFile::FromFileToFCB()
{
	Entry fcb;
	fcb.indexCluster = m_memManager->GetAddressOfFirstCluster();
	fcb.size = FileSize();
	fcb.reserved = 0;

	std::string file_name(GetFileName());
	std::string file_ext(GetExt());
	for (int i = 0; i < FNAMELEN; i++)
	{
		fcb.name[i] = ' ';
	}

	for (int i = 0; i < FEXTLEN; i++)
	{
		fcb.ext[i] = ' ';
	}

	file_name.copy(fcb.name, file_name.length());
	file_ext.copy(fcb.ext, file_ext.length());

	return fcb;
}

const char * KernelFile::GetNameFromFCB(Entry * e)
{
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

void KernelFile::SetMode(char mode)
{
	m_mode = mode;
}

void KernelFile::SetSync(ReadWrite * sync)
{
	m_sync = sync;
}
