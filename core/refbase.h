#ifndef _REFBASE_H
#define _REFBASE_H

#include <stdint.h>
#include "atomic.h"
struct refbase
{
	atomic_32_t refcount;
	uint8_t     mt;
	void (*destroyer)(void*);
};

static inline void ref_increase(struct refbase *r)
{
	if(r->mt)
		ATOMIC_INCREASE(&r->refcount);
	else
		++r->refcount;
}

static inline void ref_decrease(struct refbase *r)
{
	if(r->mt)
	{
		if(ATOMIC_DECREASE(&r->refcount) <= 0 && r->destroyer)
			r->destroyer(r);
	}
	else
	{
		if(--r->refcount <= 0 && r->destroyer)
			r->destroyer(r);
	}
}


#endif
