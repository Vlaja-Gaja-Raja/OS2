#include "file.h"
#include "KernelFile.h"
#include "ReadLock.h"
#include "ReadWrite.h"
#include "WriteLock.h"

char File::write(BytesCnt num, char* buffer)
{
	return myImpl->Write(num, buffer);
}
BytesCnt File::read(BytesCnt num, char* buffer)
{
	return myImpl->Read(num, buffer);
}
char File::seek(BytesCnt pos)
{
	return myImpl->Seek(pos);
}
BytesCnt File::filePos()
{
	return myImpl->GetPos();
}
char File::eof()
{
	return myImpl->IsEof();
}
BytesCnt File::getFileSize()
{
	return myImpl->FileSizeWithSync();
}
char File::truncate()
{
	return myImpl->Truncate();
} //** opciono
File::~File()
{
	delete myImpl;
	myImpl = 0;
} //zatvaranje fajla
File::File() {}