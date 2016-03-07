#pragma once

#include "Cluster.h"

class IClusterManager;

class BitVector :public Cluster {
public:
	// konstruktor
	BitVector(Partition* partition); // postavljanje indeksa i podatka, a modifikaciju na 0
	// destruktor
	~BitVector(); // treba da se klaster varti na disk ako je m_dirty = 1 i da se u nultom klasteru bit sa index-om postavi na 0
	// dodeljivanje vrednosti klasteru
	// postavljanje dirty bita 
	void SetIsFree(ClusterNo clusterNo, bool free);
	bool GetIsFree(ClusterNo clusterNo) const;
	ClusterNo GetIndexOfFreeCluster() const;
	// resetovanje bit vektora
	void Format();

	IClusterManager* GetClusterManager(ClusterNo firstCluster);
	IClusterManager * GetClusterManager();
};