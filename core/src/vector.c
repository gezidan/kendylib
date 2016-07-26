#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "vector.h"
#include "allocator.h"
struct vector
{
	uint32_t size;
	uint32_t capability;
	uint32_t val_size;
	int8_t     *buf;
};

struct vector *vector_create(uint32_t val_size,uint32_t reserve_size)
{
	struct vector *v = ALLOC(0,sizeof(*v));
	if(v)
	{
		if(reserve_size)
		{
			v->buf = ALLOC(0,val_size*reserve_size);
			if(!v->buf)
			{
				FREE(0,v);
				return 0;
			}
		}
		else
			v->buf = 0;
		v->size = v->capability = reserve_size;
		v->val_size = val_size;

	}
	return v;
}

struct vector *vector_copy_create(struct vector *v)
{
	assert(v);
	if(!v->buf)
		return vector_create(v->val_size,0);
	struct vector *ret = ALLOC(0,sizeof(*v));
	if(ret)
	{
		ret->buf = ALLOC(0,v->capability*v->val_size);
		if(!ret->buf)
		{
			FREE(0,ret);
			ret = 0;
		}
		else
		{
			ret->val_size = v->val_size;
			ret->size = v->size;
			ret->capability = v->capability;
			memcpy(ret->buf,v->buf,v->val_size*v->size);
		}
	}
	return ret;
}

inline void vector_clear(struct vector *v)
{
	v->size = 0;
}

void vector_copy(struct vector *v,struct vector *other)
{
	assert(v);
	assert(other);
	if(!other->buf)
		return;
	if(v->val_size == other->val_size)
	{
		if(v->capability < other->capability)
		{
			uint32_t new_size = other->capability*other->val_size;
			int8_t *buf = realloc(v->buf,new_size);
			if(!buf)
				buf = ALLOC(0,new_size);
			if(!buf)
				return;
			if(buf != v->buf)
			{
				FREE(0,v->buf);
				v->buf = buf;
			}
			v->capability = other->capability;
		}
		memcpy(v->buf,other->buf,other->size*other->val_size);
		v->size = other->size;
	}
}

void vector_destroy(struct vector **v)
{
	assert(v);
	assert(*v);
	if((*v)->buf)
		FREE(0,(*v)->buf);
	FREE(0,*v);
	*v = 0;
}

void vector_reserve(struct vector *v,uint32_t reserve_size)
{
	assert(v);
	if(v->capability > reserve_size)
	{
		//缩小容量的情况，不重新分配内存，仅仅将capality的值减小
		v->capability = reserve_size;
		if(v->size > reserve_size)
			v->size = reserve_size;
	}
	else
	{
		//需要扩大内存
		uint32_t new_size = v->val_size * reserve_size;
		int8_t *buf = realloc(v->buf,new_size);
		if(!buf)
			buf = ALLOC(0,new_size);
		if(!buf)
			return;

		v->capability = reserve_size;
		if(buf != v->buf)
		{
			memcpy(buf,v->buf,v->val_size*v->size);
			FREE(0,v->buf);
			v->buf = buf;
		}
	}
}

inline void *vector_get_addr_by_idx(struct vector *v,uint32_t idx)
{
	return (void*)&v->buf[v->val_size*idx];		
}

inline uint32_t vector_size(struct vector *v)
{
	assert(v);
	return v->size;
}

inline uint32_t vector_capability(struct vector *v)
{
	assert(v);
	return v->capability;
}

inline void* vector_to_array(struct vector *v)
{
	assert(v);
	return v->buf;
}

void vector_push_back(struct vector *v,void *val)
{
	assert(v);
	if(v->size == v->capability)
	{	
		//扩展空间
		uint32_t new_size = v->capability > 0 ? 2*(v->capability) : 32;
		uint32_t capability = new_size;
		new_size *= v->val_size;
		int8_t *buf = realloc(v->buf,new_size);
		if(!buf)
			buf = ALLOC(0,new_size);
		if(!buf)
			return;
		if(buf != v->buf)
		{
			memcpy(buf,v->buf,v->val_size*v->capability);
			FREE(0,v->buf);
			v->buf = buf;
		}
		v->capability = capability;
	}
	memcpy(vector_get_addr_by_idx(v,v->size),val,v->val_size);	
	++v->size;
}


void vector_get(struct vector *v,uint32_t idx,void *val)
{
	assert(v);
	if(v->buf && idx < v->capability)
	{
		memcpy(val,vector_get_addr_by_idx(v,idx),v->val_size);
	}
}

void vector_set(struct vector *v,uint32_t idx,void *val)
{
	assert(v);
	if(v->buf && idx < v->capability)
	{
		memcpy(vector_get_addr_by_idx(v,idx),val,v->val_size);
	}
}
