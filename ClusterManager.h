#pragma once

#include "IClusterManager.h"

#include "part.h"
#include "Cluster.h"

#include <vector>

class BitVector;

class ClusterManager : public IClusterManager
{
public:
	ClusterManager(Partition* partition, BitVector* bitVector, ClusterNo firstCluster);
	ClusterManager(Partition* partition, BitVector* bitVector);
	int GetClusterCount() const override;
	void GetCluster(int index, Cluster& cluster) override;
	void ReleaseClusters(int index) override;
	void AddNewCluster() override;
	void Load(ClusterNo firstCluster) override;
	~ClusterManager() {}
	ClusterNo GetAddressOfFirstCluster() override;
private:
	//-----------------------------------------------------------------------------------------
	bool SetAddressInFirstCluster(ClusterNo address);
	bool SetAddressInSecondCluster(ClusterNo address);
	bool SetClusterAddress(ClusterNo address);

	ClusterNo GetSecondClusterAddress(BytesCnt filePos);
	ClusterNo GetDataClusterAddress(BytesCnt filePos);

	ClusterNo ReadAddressFromEntry(EntryNum entry);
	static ClusterNo ReadAddressFromCluster(Cluster& cluster, EntryNum entry);
	BytesCnt WriteAddressInCluster(Cluster& cluster, EntryNum entry, ClusterNo address);

	static ClusterNo GetFirstIndexEntry(BytesCnt filePosition);
	static ClusterNo GetSecondIndexEntry(BytesCnt filePosition);
	static bool IsSecondIndexNeeded(BytesCnt filePosition);
	static EntryNum GetDataClusterEntry(BytesCnt filePosition);
	static BytesCnt GetPositionInDataCluster(BytesCnt filePosition);

	BytesCnt GetCapacity() const;
	//-----------------------------------------------------------------------------------------


	void IsLoad();

	bool m_IsLoad;
	BitVector* m_bitVector;
	Partition* m_partition;

	std::vector<ClusterNo> m_clusters;
	//-------------------------------------------------------
	Cluster m_firstIndex;
	Cluster m_secondIndex;

	ClusterNo m_addressOfFirstIndexCluster; // zbog odlozenog ucitavanja 
	//(kada se mauntuje ucitavanje root-a mora da se odlozi da ne bi citao neke gluposti/smece)
	// (na pozivaocu je odgovornost da formatira ili ne formatira particuju)
};