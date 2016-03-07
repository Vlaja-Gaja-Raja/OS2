#include "file.h"
#include "KernelFile.h"
#include "KernelFS.h"
#include "WriteLock.h"

KernelFS* FS::myImpl = new KernelFS();

FS::~FS() {}
char FS::mount(Partition* partition) { //montira particiju vraca dodeljeno slovo ili 0 u slucaju neuspeha
	return myImpl->Mount(partition);
}

char FS::unmount(char part) {  //demontira particiju oznacenu datim slovom vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
	return myImpl->Unmount(part);
}

char FS::format(char part) {  // formatira particiju sa datim slovom; vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
	return myImpl->Format(part);
}
char FS::readRootDir(char part, EntryNum n, Directory &d) { /*prvim argumentom se zadaje particija,
															 drugim redni broj validnog ulaza od kog se pocinje citanje,
															 treci argument je adresa na kojoj se smesta procitani niz ulaza*/
	return myImpl->readRootDir(part, n, d);
}
char FS::doesExist(char* fname) {  //argument je naziv fajla zadat apsolutnom putanjom
	return myImpl->DoesExist(fname);
}
File* FS::open(char* fname, char mode) {
	return myImpl->Open(fname, mode);
}
char FS::deleteFile(char* fname) {
	return myImpl->DeleteFile(fname);
}
FS::FS() {}