#pragma once

#include "ReadWrite.h"

class WriteLock
{
public:
	WriteLock(ReadWrite& r);
	~WriteLock();
private:
	ReadWrite& m_r;
};