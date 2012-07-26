#ifndef _REFBASE_H
#define _REFBASE_H

#include <stdint.h>
#include "atomic.h"
struct refbase
{
	atomic_32_t refcount;
	void (*destroyer)(void*);
};

void ref_increase(struct refbase*);
void ref_decrease(struct refbase*);


#endif
