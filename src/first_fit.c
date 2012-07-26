#include "first_fit.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "first_fit_define.h"

inline struct first_fit_chunk *GetPreChunk(struct first_fit_chunk *_chunk)
{
	int32_t *tmp = (int32_t*)_chunk - 2;
	uint32_t presize = (uint32_t)tmp[1];
	struct first_fit_chunk *pre = (struct first_fit_chunk*)((uint8_t*)_chunk - presize - CHUNK_HEAD);
	return pre;
}

inline struct first_fit_chunk *GetNextChunk(struct first_fit_chunk *_chunk)
{
	struct first_fit_chunk *next = (struct first_fit_chunk*)&_chunk->buf[_chunk->size];
	return next;
}

inline void SetEndSize(struct first_fit_chunk *_chunk)
{
	uint32_t size = _chunk->size;
	uint32_t *tmp = (uint32_t*)&_chunk->buf[size];
	tmp -= 1;
	*tmp = size;
}

inline void SetEndTag(struct first_fit_chunk *_chunk)
{
	uint32_t size = _chunk->size;
	int32_t tag = _chunk->tag;
	int32_t *tmp = (int32_t*)&_chunk->buf[size];
	tmp -= 2;
	*tmp = tag;
}

inline int32_t GetPreChunkTag(struct first_fit_chunk *_chunk)
{
	int32_t *tmp = (int32_t*)_chunk - 2;
	return tmp[0];
}

inline int32_t GetNextChunkTag(struct first_fit_chunk *_chunk)
{
	struct first_fit_chunk *next = GetNextChunk(_chunk);
	return next->tag;
}


struct allocator* first_fit_create(uint32_t size)
{
	struct first_fit_pool *pool = ALLOC(0,sizeof(*pool)+size-1);
	if(pool)
	{
		pool->pool_size = size;
		pool->begin = pool->buf;
		pool->end = &pool->buf[size];
		struct first_fit_chunk *first_chunk = (struct first_fit_chunk*)pool->buf;
		first_chunk->tag = FREE_TAG;
		first_chunk->size = size-CHUNK_HEAD;
		first_chunk->next = first_chunk->pre = &pool->ava_head;
		SetEndTag(first_chunk);
		SetEndSize(first_chunk);
		pool->ava_head.next = pool->ava_head.pre = first_chunk;
		pool->super_class.Alloc = first_fit_alloc;
		pool->super_class.DeAlloc = first_fit_dealloc;
		pool->super_class.Destroy = first_fit_destroy;
	}
	return (struct allocator*)pool;
}

void first_fit_destroy(struct allocator **pool)
{
	assert(pool);
	assert(*pool);
	FREE(0,*pool);
	*pool = 0;
}


void *first_fit_alloc(struct allocator *_pool,int32_t size)
{
	assert(_pool);
	if(size <= 0)
		return 0;	
	uint32_t alloc_size = (uint32_t)size;
	struct first_fit_pool *pool = (struct first_fit_pool *)_pool;
	struct first_fit_chunk *cur = pool->ava_head.next;
	void *addr = 0;
	while(cur != &pool->ava_head)
	{
		if(cur->size >= alloc_size)
		{
			cur->tag = USE_TAG;
			addr = (void*)cur->buf;
			uint32_t size_remain = cur->size - alloc_size;
			if(size_remain >= RESERVESIZE)
			{
				//拆分
				struct first_fit_chunk *other = (struct first_fit_chunk *)&cur->buf[alloc_size];
				other->size = size_remain - CHUNK_HEAD;
				other->tag = FREE_TAG;
				SetEndSize(other);
				SetEndTag(other);
				cur->size = alloc_size;				
				cur->pre->next = other;
				cur->next->pre = other;
				other->next = cur->next;
				other->pre = cur->pre;
			}
			else
			{
				//从ava中移除cur
				cur->pre->next = cur->next;
				cur->next->pre = cur->pre;
			}
			break;
		}
		cur = cur->next;
	}
	return addr;
}

void first_fit_dealloc(struct allocator *_pool,void *ptr)
{
	assert(_pool);
	if(!ptr)
		return;
	uint8_t *_ptr = (uint8_t*)ptr;
	struct first_fit_pool *pool = (struct first_fit_pool *)_pool;
	struct first_fit_chunk *P0 = (struct first_fit_chunk *)(_ptr - CHUNK_HEAD);
	P0->next = P0->pre = 0;
	if((uint8_t*)P0 < pool->begin || &P0->buf[P0->size] > pool->end)
		return;//不是pool分配出去的
				
	struct first_fit_chunk *P = 0;
	if(GetPreChunkTag(P0) == FREE_TAG)
	{
		P = GetPreChunk(P0);
		//检查前置块的首地址是否合法
		if((uint8_t*)P >= pool->begin)
		{
			//C2
			//将P从链表中脱离
			struct first_fit_chunk *P1 = P->next;
			struct first_fit_chunk *P2 = P->pre;
			P1->pre = P2;
			P2->next = P1;				
			//P0,P合并
			P->size += (P0->size + CHUNK_HEAD);
			P0 = P;
			printf("合并前块\n");
		}
	}	
	if(GetNextChunkTag(P0) == FREE_TAG)
	{
		P = GetNextChunk(P0);
		//检查后续块的尾地址是否合法
		if(&P->buf[P->size] <= pool->end)
		{
			//C4
			//将P从链表中脱离
			struct first_fit_chunk *P1 = P->next;
			struct first_fit_chunk *P2 = P->pre;
			P1->pre = P2;
			P2->next = P1;
			//P0,P合并
			P0->size += (P->size + CHUNK_HEAD);
			printf("合并后块\n");			
		}
	}	
	//将P0插入可用链表首部
	P0->next = pool->ava_head.next;
	P0->pre = &pool->ava_head;
	pool->ava_head.next->pre = P0;
	pool->ava_head.next = P0;
	P0->tag = FREE_TAG;
	SetEndSize(P0);
	SetEndTag(P0);
}

/*
uint32_t first_fit_get_free_size(struct first_fit_pool *_pool)
{
	assert(_pool);
	struct first_fit_chunk *cur = _pool->ava_head.next;
	uint32_t total_size = 0;
	while(cur != &_pool->ava_head)
	{
		total_size += (cur->size + CHUNK_HEAD);
		cur = cur->next;
		printf("do here\n");
	}
	return total_size;
}
*/
