#include "fix_obj_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static const  int8_t invalid_index = -1;
static const  uint32_t BLOCK_OBJ_SIZE = 128;
struct block
{
	int8_t m_head;
	int8_t m_tail;
	uint16_t m_freesize;
	struct block *m_next_block;
	uint32_t obj_size;//单个对象的大小
	union
	{
		int8_t m_objs[1];
		uint32_t pad;
	};
};

inline void *block_get(struct block *_block,uint32_t index)
{
	return (void*)&_block->m_objs[_block->obj_size * index];
}

inline int32_t  block_check(struct block *_block,void *obj)
{
	if((int8_t*)obj < _block->m_objs || (int8_t*)obj >= (int8_t*)block_get(_block,BLOCK_OBJ_SIZE))
		return 0;
	return 1;
}


void block_destroy(struct block **_block)
{
	FREE(0,*_block);
	*_block = 0;
}

void block_init(struct block *_block,uint32_t obj_size)
{
	_block->m_head = 0;
	_block->m_tail = BLOCK_OBJ_SIZE-1;
	_block->m_freesize = BLOCK_OBJ_SIZE;
	_block->obj_size = obj_size;
	int32_t i;
	for(i = 0; i < BLOCK_OBJ_SIZE-1; ++i)
	{
		int8_t *link = (int8_t*)block_get(_block,i);
		*link = i+1;
	}
	int8_t *link = (int8_t*)block_get(_block,BLOCK_OBJ_SIZE-1);
	*link = invalid_index;
	_block->m_next_block = 0;
}

struct block *block_create(uint32_t obj_size)
{
	struct block *_block = ALLOC(0,sizeof(*_block) + obj_size*BLOCK_OBJ_SIZE - sizeof(uint32_t));
	block_init(_block,obj_size);
	return _block;
}

void* block_alloc(struct block *_block)
{
	if(_block->m_head == invalid_index)
		return 0;

	int8_t *obj = (int8_t*)block_get(_block,_block->m_head);
	_block->m_head = *obj;

	if(_block->m_head == invalid_index)
		_block->m_tail = invalid_index;		
	*obj = invalid_index;
	--_block->m_freesize;
	return (void*)obj;
}

inline uint32_t block_get_obj_index(struct block *_block,void* obj)
{
	uint32_t offset = (int8_t*)obj - _block->m_objs;
	return offset/_block->obj_size;
}

void block_dealloc(struct block *_block,void* obj)
{
	assert(block_check(_block,obj));
	int8_t *link = (int8_t *)obj;
	*link = invalid_index;
	int8_t index = (int8_t)block_get_obj_index(_block,obj);
	if(_block->m_tail == invalid_index)
	{
		_block->m_head = index;
	}
	else
	{
		*(int8_t*)block_get(_block,_block->m_tail) = index;	
	}
	_block->m_tail = index;
	++_block->m_freesize;
}

inline uint8_t block_getfreesize(struct block *_block)
{
	return _block->m_freesize;
}

inline void block_SetNextBlock(struct block *_block,struct block *next)
{
	_block->m_next_block = next;
}

inline struct block *block_GetNext(struct block *_block)
{
	return _block->m_next_block;
}

inline int8_t *block_GetLastAddr(struct block *_block)
{
	return (int8_t*)block_get(_block,BLOCK_OBJ_SIZE-1);
}

struct chunk
{
	struct block *m_head;
	struct block *m_tail;
	uint32_t m_freesize;
	uint32_t obj_size;
	uint32_t block_count;
	uint32_t block_size;
	uint32_t idx;//供pool_alloc2和pool_dealloc2使用
	struct chunk *m_next;
	struct block m_blocks[1];
};

inline struct block *chunk_get(struct chunk *_chunk,uint32_t index)
{
	int8_t *tmp = (int8_t*)_chunk->m_blocks;
	return (struct block *)&tmp[_chunk->block_size*index];
}

void chunk_init(struct chunk *_chunk,uint32_t block_count,uint32_t block_size,uint32_t obj_size)
{
	_chunk->m_freesize = block_count*BLOCK_OBJ_SIZE;
	_chunk->block_size = block_size;
	_chunk->block_count = block_count;
	uint32_t i;
	for(i = 0; i < block_count; ++i)
	{
		block_init(chunk_get(_chunk,i),obj_size);
	}
	for(i = 0; i < block_count-1; ++i)
	{
		block_SetNextBlock(chunk_get(_chunk,i),chunk_get(_chunk,i+1));
	}
	block_SetNextBlock(chunk_get(_chunk,block_count-1),0);
	_chunk->m_head = chunk_get(_chunk,0);
	_chunk->m_tail = chunk_get(_chunk,block_count-1);
	_chunk->m_next = 0;
}

struct chunk *chunk_create(uint32_t block_count,uint32_t obj_size)
{
	uint32_t block_size = sizeof(struct block) + obj_size * BLOCK_OBJ_SIZE - sizeof(uint32_t);
	uint32_t chunk_size = block_size*block_count;
	struct chunk *_chunk = ALLOC(0,sizeof(*_chunk) + chunk_size - sizeof(_chunk->m_blocks));
	chunk_init(_chunk,block_count,block_size,obj_size);
	return _chunk;
}

void chunk_destroy(struct chunk **_chunk)
{
	FREE(0,*_chunk);
	*_chunk = 0;
}

void* chunk_alloc(struct chunk *_chunk)
{

	if(!_chunk->m_head)
		return 0;
	void *ret = block_alloc(_chunk->m_head);
	if(block_getfreesize(_chunk->m_head) == 0)
	{
		struct block *next = block_GetNext(_chunk->m_head);
		block_SetNextBlock(_chunk->m_head,0);
		if(!next)
			_chunk->m_head = _chunk->m_tail = 0;
		else
			_chunk->m_head = next;
	}
	--_chunk->m_freesize;
	return ret;				
}

inline int32_t chunk_check(struct chunk *_chunk,void *obj)
{
	if((int8_t*)obj < (int8_t*)_chunk->m_blocks || (int8_t*)obj >= (int8_t*)chunk_get(_chunk,_chunk->block_count))
		return 0;
	return 1;	
}

void chunk_dealloc(struct chunk *_chunk,void* obj)
{
	assert(chunk_check(_chunk,obj));
	int32_t index = (int32_t)(((int8_t*)obj - (int8_t*)&_chunk->m_blocks[0])/_chunk->block_size);
	block_dealloc(chunk_get(_chunk,index),obj);
	
	if(block_getfreesize(chunk_get(_chunk,index)) == 1)
	{
		if(!_chunk->m_tail)
		{
			_chunk->m_head = chunk_get(_chunk,index);
		}
		else
		{
			block_SetNextBlock(_chunk->m_tail,chunk_get(_chunk,index));
		}
		_chunk->m_tail = chunk_get(_chunk,index);
	}
	++_chunk->m_freesize;
}

inline uint32_t chunk_GetFreeSize(struct chunk *_chunk)
{
	return _chunk->m_freesize;
}

inline int8_t *chunk_GetBlocksAddr(struct chunk *_chunk)
{
	return (int8_t*)_chunk->m_blocks;
}

inline int8_t *chunk_GetLastAddr(struct chunk *_chunk)
{
	return (int8_t*)block_GetLastAddr(chunk_get(_chunk,_chunk->block_count-1));
}

inline void chunk_SetNext(struct chunk *_chunk, struct chunk * next)
{
	_chunk->m_next = next;
}

inline struct chunk *chunk_GetNext(struct chunk *_chunk)
{
	return _chunk->m_next;
}

#define DEFAULT_CHUNK_COUNT 128 //当chunk数量大于此值时将采用动态分配

struct fix_obj_pool
{
	IMPLEMEMT(allocator);
	struct chunk *m_lastDeAlloc;
	struct chunk *m_head;
	struct chunk *m_tail;
	struct chunk **ptr_chunk;
	uint32_t     chunk_count;
	uint32_t     obj_size;
	int32_t      default_size;
	struct chunk *chunks[DEFAULT_CHUNK_COUNT];
};

inline uint32_t alignsize(uint32_t obj_size)
{
	if(obj_size % 4 > 0)
		obj_size = (obj_size / 4) * 4 + 4;
	return obj_size;
}

struct allocator *create_pool(uint32_t obj_size,int32_t default_size,int32_t align4)
{
	//将default_size规整为128的倍数
	if(default_size <= 0)
		default_size = BLOCK_OBJ_SIZE;
	else
	{
		if(default_size % BLOCK_OBJ_SIZE > 0)
			default_size = (default_size / BLOCK_OBJ_SIZE) * BLOCK_OBJ_SIZE + BLOCK_OBJ_SIZE;
	}

	struct fix_obj_pool *pool = ALLOC(0,sizeof(*pool));
	if(pool)
	{
		if(align4)
			obj_size = alignsize(obj_size);
		struct chunk *_chunk = chunk_create(default_size/BLOCK_OBJ_SIZE,obj_size);
		_chunk->idx = 0;
		pool->m_lastDeAlloc = 0;
		pool->m_head = pool->m_tail = _chunk;
		pool->chunk_count = 1;
		pool->ptr_chunk = pool->chunks;
		pool->ptr_chunk[0] = _chunk;
		pool->obj_size = obj_size;
		pool->default_size = default_size;
		pool->super_class.Alloc = pool_alloc;
		pool->super_class.DeAlloc = pool_dealloc;
		pool->super_class.Destroy = destroy_pool;
	}
	return (struct allocator*)pool;
	
}

void destroy_pool(struct allocator **_pool)
{
	assert(_pool);
	assert(*_pool);
	
	struct fix_obj_pool *pool = (struct fix_obj_pool *)(*_pool);
	
	uint32_t i = 0;
	for( ; i < (pool)->chunk_count; ++i)
		chunk_destroy(&(pool->ptr_chunk[i]));
	if(pool->ptr_chunk != pool->chunks)
		FREE(0,pool->ptr_chunk);
	FREE(0,*_pool);
	*_pool = 0;
}

//供mem_allocator使用
void *pool_alloc2(struct fix_obj_pool *pool,uint16_t *chunk_idx)
{
	assert(pool);
	assert(chunk_idx);
	void *ret = chunk_alloc(pool->m_head);
	*chunk_idx = pool->m_head->idx;
	if(chunk_GetFreeSize(pool->m_head) == 0)
	{
		pool->m_head = chunk_GetNext(pool->m_head);
		if(!pool->m_head)
		{
			//没有空间了，扩展新的块
			pool->default_size = pool->default_size << 2;
			struct chunk *_chunk = chunk_create(pool->default_size/BLOCK_OBJ_SIZE,pool->obj_size);
			if(!_chunk)
				return 0;
			_chunk->idx = pool->chunk_count;
			pool->m_head = pool->m_tail = _chunk;
			pool->chunk_count++;
			if(pool->chunk_count > DEFAULT_CHUNK_COUNT)
			{
				struct chunk **tmp = ALLOC(0,sizeof(*tmp)*pool->chunk_count);
				int32_t i = 0;
				for( ; i < pool->chunk_count - 1;++i)
					tmp[i] = pool->ptr_chunk[i];
				tmp[pool->chunk_count - 1] = _chunk;
				
				if(pool->ptr_chunk != pool->chunks)
					FREE(0,pool->ptr_chunk);
				pool->ptr_chunk = tmp;
			}
			else
			{
				pool->ptr_chunk[pool->chunk_count-1] = _chunk;
			}
			
			//不需要排序了
		}
	}
	return ret;		
	
}

void* pool_alloc(struct allocator *_pool,int32_t s)
{
	assert(_pool);
	struct fix_obj_pool *pool = (struct fix_obj_pool *)_pool;
	void *ret = chunk_alloc(pool->m_head);
	if(chunk_GetFreeSize(pool->m_head) == 0)
	{
		pool->m_head = chunk_GetNext(pool->m_head);
		if(!pool->m_head)
		{
			//没有空间了，扩展新的块
			pool->default_size = pool->default_size << 2;
			struct chunk *_chunk = chunk_create(pool->default_size/BLOCK_OBJ_SIZE,pool->obj_size);
			if(!_chunk)
				return 0;
			pool->m_head = pool->m_tail = _chunk;
			pool->chunk_count++;
			if(pool->chunk_count > DEFAULT_CHUNK_COUNT)
			{
				struct chunk **tmp = ALLOC(0,sizeof(*tmp)*pool->chunk_count);
				int32_t i = 0;
				for( ; i < pool->chunk_count - 1;++i)
					tmp[i] = pool->ptr_chunk[i];
				tmp[pool->chunk_count - 1] = _chunk;
				
				if(pool->ptr_chunk != pool->chunks)
					FREE(0,pool->ptr_chunk);
				pool->ptr_chunk = tmp;
			}
			
			//对所有的chunk按其地址顺序排序
			uint32_t size = pool->chunk_count;
			int32_t i = size-2;
			for( ; i >= 0; --i)
			{
				if(pool->m_head < pool->ptr_chunk[i])
				{
					struct chunk *tmp = pool->ptr_chunk[i];
					pool->ptr_chunk[i] = pool->m_head;
					pool->ptr_chunk[i+1] = tmp;
				}
				else
				{
					pool->ptr_chunk[i+1] = pool->m_head;
					break;
				}
			}
		}
	}

	return ret;	
}

struct chunk *BinSearch(struct fix_obj_pool *pool,void *obj)
{
	int32_t beg = 0;
	int32_t end = (int32_t)pool->chunk_count;
	int32_t len = end - beg;
	while(len > 0)
	{
		int32_t mid = beg + len/2;
		
		if(chunk_check(pool->ptr_chunk[mid],obj) == 1)
			return pool->ptr_chunk[mid];
		else if((int8_t*)obj > chunk_GetBlocksAddr(pool->ptr_chunk[mid]))
			beg = mid + 1;
		else
			end = mid - 1;
		len = end - beg;
	}
	if(chunk_check(pool->ptr_chunk[beg],obj) == 1)
		return pool->ptr_chunk[beg];
	return 0;
}

//供mem_allocator使用
void pool_dealloc2(struct fix_obj_pool *pool,uint16_t chunk_idx,void* obj)
{
	assert(pool);
	assert(obj);
	
	if(chunk_idx >= pool->chunk_count)
	{
		assert(0);
		return;
	}
	
	struct chunk *_chunk = pool->ptr_chunk[chunk_idx];
	if(chunk_check(_chunk,obj) == 0)
	{
		assert(0);
		return;
	}
	chunk_dealloc(_chunk,obj);
	if(chunk_GetFreeSize(_chunk) == 1)
	{
		if(pool->m_head)
		{
			chunk_SetNext(pool->m_tail,_chunk);
			pool->m_tail = _chunk;
		}
		else
			pool->m_head = pool->m_tail = _chunk;
	}		
}

void pool_dealloc(struct allocator *_pool,void* obj)
{
	assert(_pool);
	assert(obj);
	struct fix_obj_pool *pool = (struct fix_obj_pool *)_pool;	
	struct chunk *_chunk = pool->m_lastDeAlloc;
	if(!_chunk || chunk_check(_chunk,obj) == 0)
		_chunk = BinSearch(pool,obj);
	if(_chunk)
	{
		chunk_dealloc(_chunk,obj);
		if(chunk_GetFreeSize(_chunk) == 1)
		{
			if(pool->m_head)
			{
				chunk_SetNext(pool->m_tail,_chunk);
				pool->m_tail = _chunk;
			}
			else
				pool->m_head = pool->m_tail = _chunk;
		}
		pool->m_lastDeAlloc = _chunk;
	}
}

/*
uint32_t get_free_size(struct fix_obj_pool *pool)
{
	assert(pool);
	uint32_t totalsize = 0;
	int32_t i = 0; 
	for( ; i < pool->chunk_count; ++i)
	{
		totalsize += chunk_GetFreeSize(pool->ptr_chunk[i]);
	}
	return totalsize;
}
*/
