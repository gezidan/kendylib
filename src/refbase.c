#include "refbase.h"

#ifdef MT
void ref_increase(struct refbase *r)
{
	ATOMIC_INCREASE(&r->refcount);
}

void ref_decrease(struct refbase *r)
{
	if(ATOMIC_DECREASE(&r->refcount) <= 0)
		r->destroyer(r);
}
#else

void ref_increase(struct refbase *r)
{
	++r->refcount;
}

void ref_decrease(struct refbase *r)
{
	if(--r->refcount <= 0)
		r->destroyer(r);
}
#endif
