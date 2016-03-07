#include "Semaphore.h"
#include <windows.h>


Semaphore::Semaphore(long initialCnt, long maxCnt)
{
	sem = CreateSemaphore(
		NULL,           // default security attributes
		initialCnt,  // initial count
		maxCnt,  // maximum count
		NULL);          // unnamed semaphore
	if (sem == NULL)
	{
		return;
	}
}

Semaphore::~Semaphore()
{
	CloseHandle(sem);
}

void Semaphore::signal()
{
	if (!ReleaseSemaphore(
		sem,  // handle to semaphore
		1,            // increase count by one
		NULL))       // not interested in previous count
	{
	}
}

DWORD Semaphore::wait()
{
	return WaitForSingleObject(
		sem,   // handle to semaphore
		INFINITE);// infinite time-out interval
}