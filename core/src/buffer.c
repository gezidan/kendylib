#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "buffer.h"
#include "allocator.h"

static void buffer_destroy(void *b)
{
	buffer_t _b = (buffer_t)b;
	if(_b->next)
		buffer_release(&(_b)->next);
	FREE(NULL,_b);
	b = 0;
}

static inline buffer_t buffer_create(uint8_t mt,uint32_t capacity)
{
	uint32_t size = sizeof(struct buffer) + capacity;
	buffer_t b = (buffer_t)ALLOC(NULL,size);		
	if(b)
	{
		b->size = 0;
		b->capacity = capacity;
		b->_refbase.refcount = 0;
		b->_refbase.mt = mt;
		b->_refbase.destroyer = buffer_destroy;	
	}
	return b;
}


buffer_t buffer_create_and_acquire(uint8_t mt,buffer_t b,uint32_t capacity)
{
	buffer_t nb = buffer_create(mt,capacity);
	return buffer_acquire(b,nb);
}