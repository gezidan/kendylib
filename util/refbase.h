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

inline void ref_increase(struct refbase*);
inline void ref_decrease(struct refbase*);


#endif
