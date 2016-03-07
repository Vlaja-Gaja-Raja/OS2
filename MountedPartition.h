#pragma once

class Partition;
class BitVector;
class RootDirectory;
class ReadWrite;

class MountedPartition {
private:
	static int s_id;

	Partition * m_partition;
	BitVector * m_bitVector;
	RootDirectory * m_rootDirectory;
	ReadWrite * m_sync;
	unsigned int m_NoOfOpenFiles;
	int m_id;
public:
	MountedPartition();
	~MountedPartition();
	bool Mount(Partition * partition);
	void Unmount();
	void Format();
	ReadWrite* GetSync();
	RootDirectory * GetRoot();
};