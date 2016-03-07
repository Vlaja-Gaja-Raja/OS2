#pragma once

#include "Semaphore.h"
//class Semaphore;

class ReadWrite
{
public:
	void StartRead();
	void EndRead();
	void StartWrite();
	void EndWrite();
	ReadWrite();
private:
	Semaphore e, r, w;
	int dr, dw, nr, nw;
};