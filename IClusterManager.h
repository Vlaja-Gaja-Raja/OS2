#pragma once

#include "part.h"

class Cluster;

class IClusterManager
{
public:
	virtual int GetClusterCount() const = 0;
	virtual void GetCluster(int index, Cluster& buffer) = 0;
	virtual void ReleaseClusters(int index) = 0;
	virtual void AddNewCluster() = 0;
	virtual void Load(ClusterNo firstCluster) = 0;
	virtual ~IClusterManager() {}
	virtual ClusterNo GetAddressOfFirstCluster() = 0;
};