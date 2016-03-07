#pragma once

#include <conio.h>

class Semaphore {
public:
	Semaphore(long initialCnt, long maxCnt);
	~Semaphore();
	void signal();
	unsigned long wait();
private:
	void * sem;
};