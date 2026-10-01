#include <timer.h>
#include "TimingUtilities.h"

void AddWideTo(UnsignedWide *a, UnsignedWide *res)
{
UnsignedWide r = *a;
	r.lo += res->lo;
	if (r.lo<a->lo) ++r.hi;
	r.hi += res->hi;
	*res = r;
}

void SubWide(UnsignedWide *a, UnsignedWide *b, UnsignedWide *res)
{
UnsignedWide r = *a;
	r.lo -= b->lo;
	if (r.lo>a->lo) --r.hi;
	r.hi -= b->hi;
	*res = r;
}