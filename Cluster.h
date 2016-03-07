#pragma once

#include "part.h"
#include "fs.h"

class Cluster {
public:
	// konstruktor
	Cluster(); // postavljanje indeksa i podatka, a modifikaciju na 0
	// destruktor
	~Cluster(); // treba da se klaster varti na disk ako je m_dirty = 1 i da se u nultom klasteru bit sa index-om postavi na 0
	// dodeljivanje vrednosti klasteru

	bool EndOfCluster() const;

	// Iz bafera upisi num byte-a.
	// Vraca broj uspesno upisanih bajta sa pocetka bafera.
	BytesCnt Write(char* buffer, BytesCnt num);

	// Vraca broj procitanih bajta.
	BytesCnt Read(char* buffer, BytesCnt num);

	bool WriteBack(); // vraca da li je uspeo da upise

	bool Seek(BytesCnt position);

	bool Load(Partition * partition, ClusterNo clusterAddress);

	// Upisuje sve nule.
	void Clear();

	bool IsValid()const;

	ClusterNo ClusterAddress()const;

	BytesCnt GetPosition()const;

	void Delete();

	void FillWith(char value, BytesCnt position);
	Partition * GetPartition();

protected:
	ClusterNo m_address; //indeks klastera
	bool m_dirty;
	Partition* m_partition;
	BytesCnt m_current_position;
	char m_data[ClusterSize];
};