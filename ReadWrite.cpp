#include "ReadWrite.h"

ReadWrite::ReadWrite()
	: dr(0)
	, dw(0)
	, nr(0)
	, nw(0)
	, e(1, 1)
	, r(0, 1)
	, w(0, 1)
{
}

void ReadWrite::StartRead()
{
	e.wait();
	if (nw > 0)
	{
		dr++;
		e.signal();
		r.wait();
	}
	nr++;
	if (dr > 0)
	{
		dr--;
		r.signal();
	}
	else
	{
		e.signal();
	}
}

void ReadWrite::EndRead()
{
	e.wait();
	nr--;
	if (nr == 0 && dw > 0)
	{
		dw--;
		w.signal();
	}
	else
	{
		e.signal();
	}
}

void ReadWrite::StartWrite()
{
	e.wait();
	if (nr > 0 || nw > 0)
	{
		dw++;
		e.signal();
		w.wait();
	}
	nw++;
	e.signal();
}

void ReadWrite::EndWrite()
{
	e.wait();
	nw--;
	if (dr > 0)
	{
		dr--;
		r.signal();
	}
	else
	{
		if (dw > 0)
		{
			dw--;
			w.signal();
		}
		else
		{
			e.signal();
		}
	}
}
