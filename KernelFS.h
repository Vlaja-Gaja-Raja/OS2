#pragma once

#include "fs.h"

class MountedPartition;
class File;
class Partition;
class ReadWrite;

class KernelFS {
public:
	~KernelFS();
	char Mount(Partition* partition); //montira particiju vraca dodeljeno slovo ili 0 u slucaju neuspeha

	char Unmount(char part); //demontira particiju oznacenu datim slovom vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha

	char Format(char part); // formatira particiju sa datim slovom; vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha

	char DoesExist(char* fname); //argument je naziv fajla zadat apsolutnom putanjom
	File* Open(char* fname, char mode);
	char DeleteFile(char* fname);

	char readRootDir(char part, EntryNum n, Directory &d);/*prvim argumentom se zadaje particija,
														  drugim redni broj validnog ulaza od kog se pocinje citanje,
														  treci argument je adresa na kojoj se smesta procitani niz ulaza*/

	KernelFS();
protected:
	void Delete();
	MountedPartition* m_mountPartisions[26];
	ReadWrite * m_sync;
};