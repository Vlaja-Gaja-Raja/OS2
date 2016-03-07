#pragma once

#include "ReadWrite.h"
//class ReadWrite;

class ReadLock
{
public:
	ReadLock(ReadWrite& r);
	~ReadLock();
private:
	ReadWrite& m_r;
};