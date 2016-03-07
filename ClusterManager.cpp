#include "ClusterManager.h"
#include "BitVector.h"

const ClusterNo c_emptyCluster = 0;
static const unsigned c_entrySize = 4;
static const EntryNum c_entriesPerCluster = ClusterSize / c_entrySize;
static const EntryNum c_directEntries = c_entriesPerCluster / 2;


ClusterManager::ClusterManager(Partition* partition, BitVector* bitVector, ClusterNo addressOfFirstIndexCluster)
	: m_bitVector(bitVector)
	, m_partition(partition)
	, m_IsLoad(false)
	, m_addressOfFirstIndexCluster(addressOfFirstIndexCluster)
{

}

ClusterManager::ClusterManager(Partition * partition, BitVector * bitVector)
	: m_bitVector(bitVector)
	, m_partition(partition)
	, m_IsLoad(false)
{
	const ClusterNo addressOfFirstIndexCluster = m_bitVector->GetIndexOfFreeCluster();
	m_bitVector->SetIsFree(addressOfFirstIndexCluster, false);
	m_addressOfFirstIndexCluster = addressOfFirstIndexCluster;
	m_firstIndex.Load(partition, m_addressOfFirstIndexCluster);
	m_firstIndex.Clear();
}

void ClusterManager::Load(ClusterNo addressOfFirstIndexCluster)
{
	m_firstIndex.Load(m_partition, addressOfFirstIndexCluster);
	EntryNum entry = 0;
	ClusterNo clusterId = ReadAddressFromEntry(entry);
	while (clusterId != 0)
	{
		entry = entry + 1;
		m_clusters.emplace_back(clusterId);
		clusterId = ReadAddressFromEntry(entry);
	}
	m_IsLoad = true;
}

ClusterNo ClusterManager::GetAddressOfFirstCluster()
{
	if (!m_IsLoad)
	{
		return m_addressOfFirstIndexCluster;
	}
	return m_firstIndex.ClusterAddress();
}


int ClusterManager::GetClusterCount() const
{
	return static_cast<int>(m_clusters.size());
}

void ClusterManager::GetCluster(int index, Cluster& cluster)
{
	IsLoad();
	const ClusterNo clusterId = m_clusters[index];
	if (clusterId != cluster.ClusterAddress())
	{
		cluster.Load(m_partition, clusterId);
	}
}

void ClusterManager::ReleaseClusters(int index)
{
	IsLoad();
	const ClusterNo clusterId = m_clusters[index];
	ClusterNo file_capacity = GetCapacity();
	if (index >= m_clusters.size())
	{
		return;
	}
	EntryNum positionEntry = index;
	while (positionEntry * ClusterSize < file_capacity)
	{
		ClusterNo addressOfClusterForDelete = GetDataClusterAddress(file_capacity - 1);
		EntryNum entryOfFirstLevel = GetFirstIndexEntry(file_capacity - 1);

		m_bitVector->SetIsFree(addressOfClusterForDelete, true);
		if (!IsSecondIndexNeeded(file_capacity - 1))
		{
			WriteAddressInCluster(m_firstIndex, entryOfFirstLevel, 0);
		}
		else
		{
			EntryNum entryOfSecondLevel = GetSecondIndexEntry(file_capacity - 1);
			m_secondIndex.Load(m_partition, ReadAddressFromCluster(m_firstIndex, entryOfFirstLevel));
			WriteAddressInCluster(m_secondIndex, entryOfSecondLevel, 0);
			if (0 == entryOfSecondLevel)
			{
				WriteAddressInCluster(m_firstIndex, entryOfFirstLevel, 0);
				m_bitVector->SetIsFree(m_secondIndex.ClusterAddress(), true);
			}
		}
		file_capacity -= ClusterSize;
	}
	m_clusters.erase(std::next(m_clusters.begin(), index), m_clusters.end());
}

void ClusterManager::AddNewCluster()
{
	IsLoad();
	const ClusterNo clusterId = m_bitVector->GetIndexOfFreeCluster();
	m_bitVector->SetIsFree(clusterId, false);
	SetClusterAddress(clusterId);
	m_clusters.emplace_back(clusterId);
	Cluster cluster;
	int clusterIndex = GetClusterCount() - 1;
	GetCluster(clusterIndex, cluster);
	cluster.Clear();
}

void ClusterManager::IsLoad()
{
	if (!m_IsLoad)
	{
		Load(m_addressOfFirstIndexCluster);
	}
}

//*******************************************************************************************************

EntryNum ClusterManager::GetDataClusterEntry(BytesCnt filePosition)
{
	return filePosition / ClusterSize;
}

BytesCnt ClusterManager::GetPositionInDataCluster(BytesCnt filePosition)
{
	return filePosition % ClusterSize;
}

ClusterNo ClusterManager::GetFirstIndexEntry(BytesCnt filePosition)
{
	const EntryNum dataClusterEntryOrd = GetDataClusterEntry(filePosition);
	if (!IsSecondIndexNeeded(filePosition))
	{
		return dataClusterEntryOrd;
	}
	else
	{
		return (dataClusterEntryOrd - c_directEntries) / c_entriesPerCluster + c_directEntries;
	}
}

ClusterNo ClusterManager::GetSecondIndexEntry(BytesCnt filePosition)
{
	const EntryNum dataClusterEntryOrd = GetDataClusterEntry(filePosition);
	return (dataClusterEntryOrd - c_directEntries) % c_entriesPerCluster;
}

bool ClusterManager::IsSecondIndexNeeded(BytesCnt filePosition)
{
	return GetDataClusterEntry(filePosition) >= c_directEntries;
}

ClusterNo ClusterManager::GetSecondClusterAddress(BytesCnt filePos)
{
	const ClusterNo secondIndexEntry = GetFirstIndexEntry(filePos);
	return ReadAddressFromCluster(m_firstIndex, secondIndexEntry);
}

ClusterNo ClusterManager::ReadAddressFromEntry(EntryNum entry)
{
	BytesCnt filePos = entry * ClusterSize;
	if (!IsSecondIndexNeeded(filePos))
	{
		const ClusterNo dataClusterEntry = GetDataClusterEntry(filePos);
		return ReadAddressFromCluster(m_firstIndex, dataClusterEntry);
	}
	else
	{
		ClusterNo secondIndexAddress = GetSecondClusterAddress(filePos);
		m_secondIndex.Load(m_partition, secondIndexAddress);
		const ClusterNo dataEntryInSecondIndex = GetSecondIndexEntry(filePos);
		return ReadAddressFromCluster(m_secondIndex, dataEntryInSecondIndex);
	}
}

ClusterNo ClusterManager::ReadAddressFromCluster(Cluster& cluster, EntryNum entry)
{
	cluster.Seek(entry * c_entrySize);
	ClusterNo address = 0;
	char* addressBuffer = (char*)&address;
	cluster.Read(addressBuffer, c_entrySize);
	return address;
}

BytesCnt ClusterManager::WriteAddressInCluster(Cluster& cluster, EntryNum entry, ClusterNo address)
{
	cluster.Seek(entry * c_entrySize);
	char* addressBuffer = (char*)&address;
	return cluster.Write(addressBuffer, c_entrySize);
}

ClusterNo ClusterManager::GetDataClusterAddress(BytesCnt filePos)
{
	if (!IsSecondIndexNeeded(filePos))
	{
		const ClusterNo dataClusterEntry = GetDataClusterEntry(filePos);
		return ReadAddressFromCluster(m_firstIndex, dataClusterEntry);
	}
	else
	{
		ClusterNo secondIndexAddress = GetSecondClusterAddress(filePos);
		m_secondIndex.Load(m_partition, secondIndexAddress);
		const ClusterNo dataEntryInSecondIndex = GetSecondIndexEntry(filePos);
		return ReadAddressFromCluster(m_secondIndex, dataEntryInSecondIndex);
	}
}

BytesCnt ClusterManager::GetCapacity() const
{
	return m_clusters.size() * ClusterSize;
}

bool ClusterManager::SetAddressInFirstCluster(ClusterNo address) {
	// Nadji ulaz u koji treba da se upise.
	ClusterNo file_capacity = GetCapacity();
	const ClusterNo dataClusterEntry = GetFirstIndexEntry(file_capacity); // Upisi u prvi klaster datu addresu u ulaz dataClusterEntry
	BytesCnt writeBytes = WriteAddressInCluster(m_firstIndex, dataClusterEntry, address);
	return writeBytes == c_entrySize;
	//return writeBytes = c_entrySize;
}

bool ClusterManager::SetAddressInSecondCluster(ClusterNo address) {
	bool needToClear = false;
	ClusterNo file_capacity = GetCapacity();
	ClusterNo firstIndexEntry = GetFirstIndexEntry(file_capacity);
	ClusterNo firstAddress = ReadAddressFromCluster(m_firstIndex, firstIndexEntry);
	if (firstAddress == 0) {
		firstAddress = m_bitVector->GetIndexOfFreeCluster();
		m_bitVector->SetIsFree(firstAddress, false);
		if (!SetAddressInFirstCluster(firstAddress))
		{
			return false;
		}
		needToClear = true;
	}
	m_secondIndex.Load(m_partition, firstAddress);
	if (needToClear)
	{
		m_secondIndex.Clear();
	}
	file_capacity = GetCapacity();
	ClusterNo secondIndexEntry = GetSecondIndexEntry(file_capacity);
	BytesCnt writeBytes = WriteAddressInCluster(m_secondIndex, secondIndexEntry, address);
	return writeBytes == c_entrySize;
}

bool ClusterManager::SetClusterAddress(ClusterNo address) {
	// Da li novom klasteru trebaju dva undex-a.
	ClusterNo file_capacity = GetCapacity();
	if (!IsSecondIndexNeeded(file_capacity)) {
		// Postavi address u prvom indeksu.
		return SetAddressInFirstCluster(address);
	}
	else {
		// Postavi adresu u drugom indeksu.
		return SetAddressInSecondCluster(address);
	}
}