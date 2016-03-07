#pragma once

#include "part.h"
#include "fs.h"
#include "Cluster.h"
#include "BitVector.h"
#include <string>

class IClusterManager;
class RootDirectory;
class ReadWrite;
class ReadLock;
class WriteLock;

class KernelFile {
public:
	KernelFile(RootDirectory * root, BitVector * bitVector, ClusterNo addressOfFirstCluster
		, BytesCnt fileSize, char* name, char mode);
	KernelFile(RootDirectory * root, BitVector * bitVector, char* name, char mode);
	KernelFile(RootDirectory * root, BitVector * bitVector, Entry fcb, char mode);
	BytesCnt FileSizeWithSync() const;
	~KernelFile();
	bool Seek(BytesCnt bytesCnt);
	void SeekEnd();
	BytesCnt GetPos() const;
	bool IsEof() const;
	BytesCnt Write(BytesCnt bytesCnt, char * buffer);
	BytesCnt Read(BytesCnt bytesCnt, char * buffer);
	BytesCnt FileSize()const;
	bool Truncate();
	void Update();
	ReadWrite * GetSync();

	void LockFileForRead();
	void LockFileForWrite();
private:
	void LoadCluster(BytesCnt position);
	ClusterNo GetClusterIndex(BytesCnt position);
	BytesCnt GetPoistionInCluster(BytesCnt position);
	
	BytesCnt GetCapacity() const;
	bool Delete(BytesCnt position);
//---------------------------------------------
	void SetName(const char* name);//done
	const std::string GetExt() const;//done
	const char* GetName() const;//done
	const std::string GetFileName() const;//done

	Entry FromFileToFCB();
	const char* GetNameFromFCB(Entry * e);
	void SetMode(char mode);
	void SetSync(ReadWrite* sync);
//----------------------------------------------
	Partition* m_partition;

	Cluster m_data;

	BytesCnt m_file_size;
	BytesCnt m_file_position;
	//BytesCnt m_file_capacity;

	std::string m_name;
	char m_mode;

	IClusterManager * m_memManager;
	RootDirectory * m_root;

	ReadWrite * m_sync;
	ReadLock * m_readLock;
	WriteLock * m_writeLock;

	friend class RootDirectory;
};