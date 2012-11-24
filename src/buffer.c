#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "buffer.h"
#include "allocator.h"

static inline void buffer_destroy(void *b)
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

buffer_t buffer_acquire(buffer_t b1,buffer_t b2)
{
	if(b1 == b2)
		return b1;	
	if(b2)
		ref_increase(&b2->_refbase);
	if(b1)
		buffer_release(&b1);

	return b2;
}

void buffer_release(buffer_t *b)
{
	if(*b)
	{
		ref_decrease(&(*b)->_refbase);
		*b = 0;
	}
}

int buffer_read(buffer_t b,uint32_t pos,int8_t *out,uint32_t size)
{
	uint32_t copy_size;
	while(size)
	{
		if(!b)
			return -1;
		copy_size = b->size - pos;
		copy_size = copy_size > size ? size : copy_size;
		memcpy(out,b->buf + pos,copy_size);
		size -= copy_size;
		pos += copy_size;
		out += copy_size;
		if(pos >= b->size)
		{
			pos = 0;
			b = b->next;
		}
	}
	return 0;
}


/*
extern uint8_t GetK(uint32_t);

static buffer_t buf_alloc(struct buffer_mgr *bm,int32_t size)
{
	buffer_t bf = NULL;
	if(bm->mtx)
		spin_lock(bm->mtx,4000);
	if(!bm->free_size)
	{
		int32_t i = 0;
		for(; i < bm->factor;)
		{
			int32_t j = 0;
			struct _mem_block *bb = calloc(1,sizeof(*bb));
			if(!bb)
				break;
			struct buffer_block *_b = calloc(bm->create_block_size,size);
			if(!_b)
			{
				free(bb);
				break;
			}
			bb->mem_block = (void*)_b;
			LINK_LIST_PUSH_BACK(bm->blocks,bb);
			for( ; j < 	bm->create_block_size; ++j)
			{				
				
				list_node *b = (list_node*)(((uint8_t*)_b) + size*j);
				if(bm->tail)
				{
					bm->tail->next = b;
					bm->tail = bm->tail->next;
				}
				else
				{
					bm->head = bm->tail = b;
				}
			}
			i += j;
			if(j != bm->create_block_size)
				break;
		}
		bm->factor = (bm->factor << 1);
		bm->free_size += i;		
	}
	
	if(bm->free_size)
	{
		bf = (buffer_t)bm->head;
		bm->head = bm->head->next;
		if(!bm->head)
		{
			bm->tail = NULL;
		}
		--bm->free_size;
	}
	if(bm->mtx)
		spin_unlock(bm->mtx);
	return bf;
}

static void buf_dealloc(struct buffer_mgr *bm,buffer_t bf)
{
	struct buffer_block *b = (struct buffer_block*)bf;
	b->next.next = NULL;
	if(bm->mtx)
		spin_lock(bm->mtx,4000);	
    if(bm->tail)
    {
		bm->tail->next = (list_node*)b;
		bm->tail = bm->tail->next;
	}
	else
	{
		bm->head = bm->tail = (list_node*)b;
	}
	++bm->free_size;
	if(bm->mtx)
		spin_unlock(bm->mtx);					
}

static void destroy_all_buf(struct buffer_mgr *bm)
{
	struct _mem_block *bb;
	while(bb = (struct _mem_block*)link_list_pop(bm->blocks))
	{
		free(bb->mem_block);
		free(bb);
	}
	if(bm->mtx)
		spin_destroy(&bm->mtx);
	LINK_LIST_DESTROY(&(bm->blocks));		
}

static void* buffer_alloc(struct allocator *a, int32_t size)
{
	uint8_t k = GetK(size);
#if _DEBUG_	
	if(k < 4)
		k = 4;
	if(k > 16)
		return NULL;
#endif		
	size = 1<<k;
	k -= 4;
	buffer_allocator_t ba = (buffer_allocator_t)a;
	void *ptr = (void*)buf_alloc(&(ba->bf_mgr[k]),size);
	return ptr;
}

static void  buffer_dealloc(struct allocator*a, void *ptr)
{
	buffer_t bf = (buffer_t)ptr;
#if _DEBUG_	
	if(bf->capacity < 16 || bf->capacity > 65536)
		return;
#endif	
	int32_t size = bf->capacity + sizeof(*bf);
	uint8_t k = GetK(size);
	k-=4;
	buffer_allocator_t ba = (buffer_allocator_t)a;
	buf_dealloc(&(ba->bf_mgr[k]),bf);
}
	
static void destroy_buffer_allocator(struct allocator **a)
{
   buffer_allocator_t ba = (buffer_allocator_t)(*a);
   int i = 0;
   for( ; i < 11; ++i)
   {
 	 destroy_all_buf(&(ba->bf_mgr[i]));
   }
   free(ba);
   *a = NULL;				
}


#define INIT_BUFFER_SIZE 256

buffer_allocator_t create_buffer_allocator(int8_t mt)
{
	buffer_allocator_t ba = calloc(1,sizeof(*ba));
	if(ba)
	{
		int i = 0;
		for( ; i < 11; ++i)
		{
			ba->bf_mgr[i].factor = INIT_BUFFER_SIZE;
			ba->bf_mgr[i].free_size = 0;
			ba->bf_mgr[i].head = ba->bf_mgr[i].tail = NULL;
			if(i<8)
				ba->bf_mgr[i].create_block_size = 256;
			else
				ba->bf_mgr[i].create_block_size = 32;
			ba->bf_mgr[i].blocks = LINK_LIST_CREATE();
			if(mt)
				ba->bf_mgr[i].mtx = spin_create();
		}
		
		ba->super_class.Alloc = buffer_alloc;
		ba->super_class.DeAlloc = buffer_dealloc;
		ba->super_class.Destroy = destroy_buffer_allocator;
	}
	return ba;
}
*/
