#include "local_pool.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

struct local_pool
{
	IMPLEMEMT(allocator);
	uint32_t free_size;
	int32_t  free_buf_in_destroy;
	int8_t *buf_ptr;
	int8_t *free_ptr;
};

struct allocator* local_pool_create(void *buf,int32_t size)
{
	local_pool_t lp = ALLOC(0,sizeof(*lp));
	if(!lp)
		return 0;
	if(buf)
	{
		lp->buf_ptr = buf;
		lp->free_buf_in_destroy = 1;
	}
	else
	{
		lp->buf_ptr = ALLOC(0,size);
		if(!lp->buf_ptr)
		{
			FREE(0,lp);
			return 0;
		}
		lp->free_buf_in_destroy = 0;
	}
	lp->free_ptr = lp->buf_ptr;
	lp->free_size = size;
	lp->super_class._alloc = local_pool_alloc;
	lp->super_class._dealloc = local_pool_dealloc;
	lp->super_class._destroy = local_pool_destroy;
	return (struct allocator*)lp;
}

void local_pool_destroy(struct allocator **_lp)
{
	assert(_lp);
	assert(*_lp);
	
	struct local_pool *lp = (struct local_pool *)(*_lp);
	
	if(!lp->free_buf_in_destroy)
		FREE(0,lp->buf_ptr);
	FREE(0,*_lp);
	*_lp = 0;
}

uint32_t alignsize(uint32_t obj_size)
{
	if(obj_size % 4 > 0)
		obj_size = (obj_size / 4) * 4 + 4;
	return obj_size;
}

void *local_pool_alloc(struct allocator *_lp,int32_t size)
{
	assert(_lp);
	struct local_pool *lp = (struct local_pool *)_lp;
	uint32_t alloc_size;
	if(size <= 0)
		alloc_size = 1;
	alloc_size = alignsize(alloc_size);
	if(lp->free_size < alloc_size)
		return 0;
	void *ret = (void*)lp->free_ptr;
	lp->free_size -= alloc_size;
	lp->free_ptr += alloc_size;
	return ret;
}

void  local_pool_dealloc(struct allocator *lp,void *ptr)
{
	//什么也不做
}
