#include "Cluster.h"
#include <stdlib.h>
#include <iostream>
#include <conio.h>
using namespace std;

// konstruktor
Cluster::Cluster() :
	m_address(0),
	m_partition(NULL),
	m_dirty(false),
	m_current_position(0) {
}

bool Cluster::EndOfCluster() const {
	return m_current_position >= ClusterSize;
}

bool Cluster::IsValid() const {
	return m_partition != NULL;
}

// u claster upisi end bajtova iz datog bafera
BytesCnt Cluster::Write(char* buffer, BytesCnt num)
{
	if (!IsValid()) {
		// Zapravo greska.
		return 0;
	}

	m_dirty = true;
	BytesCnt writtenBytes = 0;

	for (BytesCnt index = 0; !EndOfCluster() && index < num; index++) {
		m_data[m_current_position] = buffer[index];
		m_current_position++;
		writtenBytes++;
	}

	return writtenBytes;
}

// u dati bafer ucitaj end bajtova iz klastera
BytesCnt Cluster::Read(char* buffer, BytesCnt num) {
	if (!IsValid()) {
		return 0;
	}

	BytesCnt readBytes = 0;

	for (BytesCnt index = 0; index < num && !EndOfCluster(); index++) {
		buffer[index] = m_data[m_current_position];
		m_current_position++;
		readBytes++;
	}
	return readBytes;
}

bool Cluster::Seek(BytesCnt position) {
	if (position >= ClusterSize) {
		return false;
	}
	m_current_position = position;
	return true;
}

bool Cluster::WriteBack() {
	if (!IsValid())
	{
		return false;
	}

	if (m_dirty) {
		if (0 == m_partition->writeCluster(m_address, m_data)) {
			return false;
		}
		else {
			m_dirty = false;
			return true;
		}
	}
	else
	{
		return true;
	}
}

// ucitaj klaster sa date particije sa date adrese
bool Cluster::Load(Partition * partition, ClusterNo clusterAddress) {

	if ((m_partition == partition) && (m_address == clusterAddress)) {
		// Vec ucitano.
		return true;
	}

	if (!WriteBack())
	{
		// Nije uspeo upis prethodnih podataka, operacija neuspesna;
//		return false;
	}

	m_partition = NULL;
	m_address = 0;

	if (0 == partition->readCluster(clusterAddress, m_data))
	{
		// Citanje neuspesno.
		return false;
	}

	m_partition = partition;
	m_address = clusterAddress;
	m_current_position = 0;
	m_dirty = false;

	return true;
}

void Cluster::Delete()
{
	WriteBack();
	m_partition = NULL;
	m_address = 0;
	m_current_position = 0;
	m_dirty = false;
}

Cluster::~Cluster()
{
	Delete();
}

void Cluster::Clear() {
	FillWith(0, 0);
}

void Cluster::FillWith(char value, BytesCnt position)
{
	memset(m_data + position, value, ClusterSize - position);
	m_dirty = true;
	m_current_position = 0;
}

Partition * Cluster::GetPartition()
{
	return m_partition;
}

ClusterNo Cluster::ClusterAddress()const
{
	return m_address;
}

BytesCnt Cluster::GetPosition()const
{
	return m_current_position;
}