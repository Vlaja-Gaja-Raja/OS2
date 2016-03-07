#include "WriteLock.h"

WriteLock::WriteLock(ReadWrite& r) : m_r(r)
{
	m_r.StartWrite();
}

WriteLock::~WriteLock()
{
	m_r.EndWrite();
}