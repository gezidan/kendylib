#include "mem_allocator.h"
#include "first_fit.h"
#include "first_fit_define.h"
#include "fix_obj_pool.h"
#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>


#define FIRST_FIT_DEFAULT_SIZE 1024*4//1024*1024*16 //默认first_fit分配器大小是16MB

/*
* fix_obj_pool用于分配从1-1024字节大小的请求
* 请求对齐到4字节.所以总共有1024/4 = 256种大小
*/

struct general_allocator
{
	IMPLEMEMT(allocator);
	uint32_t max_request; //可以处理的最大分配请求
	struct fix_obj_pool *fix_objs[1024/4];
	struct vector *first_fits;//存放first_fit
	int32_t last_dealloc_idx;
};

struct head_fix
{
	uint16_t idx;/*0-255,表示在fix_objs的下标*/
	uint16_t chunk_idx;
};

struct head_first_fit
{
	uint32_t idx;//在first_fits中的下标
};

struct chunk;
extern void *pool_alloc2(struct fix_obj_pool*,uint16_t *chunk_idx);
extern void pool_dealloc2(struct fix_obj_pool *pool,uint16_t chunk_idx,void* obj);

struct allocator *gen_allocator_create(uint32_t max)
{
	struct general_allocator *al = ALLOC(0,sizeof(*al));
	if(!al)
		return 0;
	memset(al,sizeof(*al),0);
	al->max_request = max;
	al->last_dealloc_idx = -1;
	al->first_fits = 0;
	int32_t i = 0;
	for(; i < 1024/4; ++i)
	{
		uint32_t real_size = (i+1)*4 + sizeof(struct head_fix);
		al->fix_objs[i] = (struct fix_obj_pool*)create_pool(real_size,1024,1);
		if(!al->fix_objs[i])
		{
			gen_allocator_destroy((allocator_t*)&al);
			return 0;
		}
	}
	
	al->first_fits = vector_create(sizeof(struct first_fit_pool*),0);
	struct first_fit_pool *first_fit = (struct first_fit_pool *)first_fit_create(FIRST_FIT_DEFAULT_SIZE);
	if(!first_fit)
	{
		gen_allocator_destroy((allocator_t*)&al);
		return 0;
	}
	al->super_class.Alloc = gen_allocator_alloc;
	al->super_class.DeAlloc = gen_allocator_dealloc;
	al->super_class.Destroy = gen_allocator_destroy;
	
	VECTOR_PUSH_BACK(struct first_fit_pool*,al->first_fits,first_fit);
	return (struct allocator*)al;
}

void   gen_allocator_destroy(struct allocator **_al)
{
	assert(_al);
	assert(*_al);
	struct general_allocator *al = (struct general_allocator *)(*_al);
	uint32_t i = 0;
	if(al->first_fits)
	{	
		uint32_t size = vector_size(al->first_fits);
		struct first_fit_pool **first_fits = VECTOR_TO_ARRAY(struct first_fit_pool *,al->first_fits);
		for( ; i < size; ++i)
			DESTROY((struct allocator**)&first_fits[i]);
			//first_fit_destroy((struct allocator**)&first_fits[i]);
	}
	
	i = 0;
	for( ; i < 1024/4;++i)
	{
		if(al->fix_objs[i])
			DESTROY((struct allocator**)&al->fix_objs[i]);
			//destroy_pool((struct allocator**)&al->fix_objs[i]);
	}
	
	FREE(0,*_al);
	*_al = 0;
}

void *gen_allocator_alloc(struct allocator *_al,int32_t size)
{
	assert(_al);
	uint32_t alloc_size = size;
	if(size <= 0)
		alloc_size = 1;
	//将请求大小调整为4的倍数
	alloc_size = alignsize(alloc_size); 	
	struct general_allocator *al = (struct general_allocator *)_al;
	if(alloc_size > al->max_request)
		return 0;//无法处理这个请求
	if(alloc_size > 1024)
	{
		struct head_first_fit *hff = 0;
		//交给first fit分配器
		alloc_size += sizeof(struct head_first_fit);
		if(al->last_dealloc_idx >= 0)
		{
			//首先尝试从上次释放的first_fit分配
			struct first_fit_pool *ff = VECTOR_GET(struct first_fit_pool *,al->first_fits,al->last_dealloc_idx);
			assert(ff);
			hff = (struct head_first_fit*)first_fit_alloc((struct allocator*)ff,alloc_size);
			hff->idx = al->last_dealloc_idx;
		}
		
		if(!hff)
		{
			//遍历first_fits
			struct first_fit_pool **ffs = VECTOR_TO_ARRAY(struct first_fit_pool *,al->first_fits);
			uint32_t i = 0;
			uint32_t end = vector_size(al->first_fits);
			for( ; i < end; ++i)
			{
				hff = first_fit_alloc((struct allocator*)ffs[i],alloc_size);
				if(hff)
				{
					hff->idx = i;
					break;
				}
			}
			if(!hff)
			{
				//分配一个新的first_fit;
				struct first_fit_pool *ff = (struct first_fit_pool *)first_fit_create(FIRST_FIT_DEFAULT_SIZE);
				if(!ff)
					return 0;
				VECTOR_PUSH_BACK(struct first_fit_pool*,al->first_fits,ff);
				hff = first_fit_alloc((struct allocator*)ff,alloc_size);
				hff->idx = end;
			}
			
			void *ret = (void*)((int8_t*)hff + sizeof(*hff)); 
			return ret;
		}
	}
	else
	{
		//由fix_obj_pool处理
		uint8_t idx = alloc_size/4 - 1;
		uint16_t chunk_idx;
		struct head_fix *hf = pool_alloc2(al->fix_objs[idx],&chunk_idx);
		if(!hf)
			return 0;
		hf->idx = idx;
		hf->chunk_idx = chunk_idx;
		void *ret = (void*)((int8_t*)hf + sizeof(*hf)); 
		return ret;
	}	
}

void  gen_allocator_dealloc(struct allocator *_al,void *ptr)
{
	assert(_al);
	assert(ptr);
	struct first_fit_chunk dummy;
	struct general_allocator *al = (struct general_allocator *)_al;
	//首先确定是哪种类型的分配器分配的
	struct first_fit_chunk *ffc = (struct first_fit_chunk *)((int32_t*)ptr - sizeof(struct head_first_fit)/4 - sizeof(dummy.tag)/4 - sizeof(dummy.size)/4);
	if(ffc->tag == USE_TAG)
	{
		//由first_fit分配的
		struct head_first_fit *hff = (struct head_first_fit *)((int32_t*)ptr - sizeof(struct head_first_fit)/4);
		if(hff->idx < vector_size(al->first_fits))
		{
			struct first_fit_pool *ff = VECTOR_GET(struct first_fit_pool *,al->first_fits,hff->idx);
			assert(ff);
			first_fit_dealloc((struct allocator*)ff,hff);
			al->last_dealloc_idx = hff->idx;
		}
	}
	else
	{
		struct head_fix *hf = (struct head_fix *)((int32_t*)ptr - sizeof(struct head_fix)/4);
		if(hf->idx <= 255)
		{
			pool_dealloc2(al->fix_objs[hf->idx],hf->chunk_idx,(void*)hf);
		}
	}
}

/*
void show_fix_info(struct allocator *al)
{
	uint32_t i;
	for(i = 0; i < 1024/4;++i)
	{
		printf("size: %d,free count:%d\n",(i+1)*4,get_free_size(al->fix_objs[i]));
	}
}

void show_first_fit_info(struct allocator *al)
{
    uint32_t i;
	uint32_t size = vector_size(al->first_fits);
	struct first_fit_pool **ff = VECTOR_TO_ARRAY(struct first_fit_pool *,al->first_fits);
	for( i=0; i < size; ++i)
	{
		printf("first fit:%d,free size:%d\n",i,first_fit_get_free_size(ff[i]));
	}
}
*/
