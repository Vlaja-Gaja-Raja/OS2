#include "MountedPartition.h"
#include "BitVector.h"
#include "RootDirectory.h"
#include "WriteLock.h"

int MountedPartition::s_id = 0;

MountedPartition::MountedPartition()
	: m_partition(0)
	, m_bitVector(0)
	, m_rootDirectory(0)
{
	m_sync = new ReadWrite();
}

MountedPartition::~MountedPartition() {
	delete m_rootDirectory;
	m_rootDirectory = 0;
	m_partition = 0;
	delete m_bitVector;
	m_bitVector = 0;
	delete m_sync;
	m_sync = nullptr;
	m_NoOfOpenFiles = 0;
	s_id -= 1;
	m_id = 0;
}

bool MountedPartition::Mount(Partition* partition)
{
	if (s_id >= 26 && m_partition != 0)
		return false;
	m_partition = 0;
	delete m_bitVector;
	m_bitVector = 0;
	delete m_rootDirectory;
	m_rootDirectory = 0;
	if (partition != 0)
	{
		m_partition = partition;
		m_bitVector = new BitVector(m_partition);
		m_rootDirectory = new RootDirectory(m_bitVector);
		m_id = s_id;
		s_id++;
	}
	return true;
}

void MountedPartition::Unmount() {
	m_rootDirectory->WaitToCloseAllFiles();
	delete m_rootDirectory;
	m_rootDirectory = 0;
	m_partition = 0;
	m_bitVector = 0;
}

void MountedPartition::Format()
{
	m_rootDirectory->WaitToCloseAllFiles();
	RootDirectory* rootDir = GetRoot();
	WriteLock lockRoot(*rootDir->GetSync());
	m_bitVector->Format();
	m_rootDirectory->Format();
}

ReadWrite * MountedPartition::GetSync()
{
	return m_sync;
}

RootDirectory * MountedPartition::GetRoot()
{
	return m_rootDirectory;
}
