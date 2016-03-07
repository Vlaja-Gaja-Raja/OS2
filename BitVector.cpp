#include "BitVector.h"
#include "KernelFsExceptions.h"
#include "ClusterManager.h"

static const ClusterNo c_maxNumberOfClusters = ClusterSize * 8;

//konstruktor
BitVector::BitVector(Partition* partition) :Cluster() {
	Load(partition, 0);
}

BitVector::~BitVector() {
}

void BitVector::SetIsFree(ClusterNo clusterNo, bool free) {
	Verify(clusterNo < c_maxNumberOfClusters);

	const BytesCnt byteInsideCluster = clusterNo / 8;
	const BytesCnt bitInsideByte = clusterNo % 8;

	if (free)
	{
		const char mask = 1 << (bitInsideByte);
		m_data[byteInsideCluster] = m_data[byteInsideCluster] | mask;
	}
	else
	{
		char mask = 1 << (bitInsideByte);
		mask = ~mask;
		m_data[byteInsideCluster] = m_data[byteInsideCluster] & mask;
	}

	m_dirty = true;
}

bool BitVector::GetIsFree(ClusterNo clusterNo) const
{
	Verify(clusterNo < c_maxNumberOfClusters);
	const BytesCnt byteInsideCluster = clusterNo / 8;
	const BytesCnt bitInsideByte = clusterNo % 8;
	const char mask = 1 << (bitInsideByte);
	return (0x00 != (m_data[byteInsideCluster] & mask));
}

ClusterNo BitVector::GetIndexOfFreeCluster() const {
	ClusterNo freeCluster = 0;
	for (ClusterNo index = 2; index < c_maxNumberOfClusters; index++)
	{
		if (GetIsFree(index))
		{
			freeCluster = index;
			break;
		}
	}

	Verify(freeCluster < c_maxNumberOfClusters);
	return freeCluster;
}

void BitVector::Format() {
	FillWith(0xFF, 0);
	SetIsFree(0, false);
	SetIsFree(1, false);
}

IClusterManager * BitVector::GetClusterManager(ClusterNo firstCluster)
{
	if (firstCluster != 0)
	{
		return new ClusterManager(m_partition, this, firstCluster);
	}
	return new ClusterManager(m_partition, this);
}

IClusterManager * BitVector::GetClusterManager()
{
	return new ClusterManager(m_partition, this);
}
