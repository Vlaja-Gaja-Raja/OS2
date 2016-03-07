#include "ReadLock.h"

ReadLock::ReadLock(ReadWrite& r) : m_r(r)
{
	m_r.StartRead();
}

ReadLock::~ReadLock()
{
	m_r.EndRead();
}