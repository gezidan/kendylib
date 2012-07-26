#include "block_obj_allocator.h"
#include <pthread.h>
#include "link_list.h"
#include <stdint.h>
#include <assert.h>
#include "spinlock.h"
#include <stdlib.h>
#include "allocator.h"



struct free_list{
	list_node next;
	uint32_t  size;
	uint32_t  init_size;
	list_node *head;
	list_node *tail;
	void *mem;
};

struct thread_allocator
{
	list_node next;
	block_obj_allocator_t central_allocator;
	struct link_list *_free_list;
	struct link_list *_recover;
	uint32_t   free_size;
	uint32_t   collect_factor;
};

struct block_obj_allocator
{
	IMPLEMEMT(allocator);
	pthread_key_t t_key;
	struct link_list *_free_list;
	spinlock_t mtx;
	struct link_list *_thread_allocators;
	uint32_t obj_size;
};

static inline void *free_list_get(struct free_list *f)
{
	void *ptr = (void*)f->head;
	f->head = f->head->next;
	if(!f->head)
		f->tail = NULL;
	--f->size;	
	return ptr;
}

static inline void free_list_put(struct free_list *f,void *ptr)
{
	list_node *l = (list_node*)ptr;
	l->next = NULL;
	if(f->tail)
	{
		f->tail->next = l;
		f->tail = l;
	}
	else
		f->head = f->tail = l;
	++f->size;
}

#define DEFAULT_BLOCK_SIZE 1024*1024

static struct free_list *creat_new_freelist(uint32_t size)
{
	uint32_t init_size = DEFAULT_BLOCK_SIZE/size;
	struct free_list *f = (struct free_list*)calloc(1,sizeof(*f));
	assert(f);
	f->mem = calloc(1,DEFAULT_BLOCK_SIZE);
	assert(f->mem);
	f->init_size = f->size = init_size;
	int32_t i = 0;
	for( ; i < init_size; ++i)
	{
		list_node *l = (list_node*)(((uint8_t*)f->mem)+(i*size));
		free_list_put(f,l);
	}
	f->size = init_size;
	return f;	
}

static struct free_list *central_get_freelist(block_obj_allocator_t central)
{
	//printf("central_get_freelist\n");
	struct free_list *f;
	spin_lock(central->mtx,4000);
	f = (struct free_list*)link_list_pop(central->_free_list);
	spin_unlock(central->mtx);
	if(!f)
	{
		//printf("creat_new_freelist\n");
 	    f = creat_new_freelist(central->obj_size);			
	}
	return f;
}

static void give_back_to_central(block_obj_allocator_t central,struct free_list *f)
{
	//printf("give_back_to_central\n");
	spin_lock(central->mtx,4000);
	LINK_LIST_PUSH_BACK(central->_free_list,f);
	spin_unlock(central->mtx);
}

void *thread_allocator_alloc(struct thread_allocator *a)
{
	void *ptr;
	struct free_list *f;
	if(!a->free_size)
	{
		//thread cache不够内存了，从central获取
		f = central_get_freelist(a->central_allocator);
		assert(f);
		LINK_LIST_PUSH_BACK(a->_free_list,f);
		a->free_size += f->size;
	}
	else
	{
		f = (struct free_list*)link_list_head(a->_free_list);
		if(!f)
		{
			f = (struct free_list*)link_list_pop(a->_recover);
			LINK_LIST_PUSH_BACK(a->_free_list,f);
		}
	}
	ptr = free_list_get(f);
	assert(ptr);
	--a->free_size;
	if(!f->size)
	{
		link_list_pop(a->_free_list);
		link_list_push_back(a->_recover,(list_node*)f);
	}
	return ptr;
}

void thread_allocator_dealloc(struct thread_allocator *a,void *ptr)
{
	struct free_list *f = (struct free_list*)link_list_head(a->_recover);
	if(f)
	{
		free_list_put(f,ptr);
		++a->free_size;
		if(f->size == f->init_size)
		{
			link_list_pop(a->_recover);	
			//printf("==init_size\n");
			//一个free_list回收满了,要么放到free_list中，要么归还central
			if(a->free_size >= a->collect_factor)
			{
				//将f归还给central_allocator;	
				give_back_to_central(a->central_allocator,f);
				a->free_size -= f->size;
			}
			else
				link_list_push_back(a->_free_list,(list_node*)f);
		}
	}
	else
	{
		f = (struct free_list*)link_list_head(a->_free_list);
		assert(f);
		free_list_put(f,ptr);
		++a->free_size;
	}
}

static void release_freelist(struct link_list *flist)
{
	list_node *l = link_list_head(flist);
	while(l)
	{
		struct free_list *f = (struct free_list*)l;
		l = l->next;
		free(f->mem);
		free(f);
		//printf("destroy_freelist\n");
	}	
}

void destroy_thread_allocator(struct thread_allocator *a)
{
	release_freelist(a->_free_list);
	release_freelist(a->_recover);
	LINK_LIST_DESTROY(&(a->_free_list));
	LINK_LIST_DESTROY(&(a->_recover));
	free(a);
}

struct thread_allocator *create_thread_allocator(block_obj_allocator_t ba)
{
	struct thread_allocator *a = (struct thread_allocator*)calloc(1,sizeof(*a));
	if(a)
	{
		a->central_allocator = ba;
		a->_free_list = LINK_LIST_CREATE();
		a->_recover = LINK_LIST_CREATE();
		a->collect_factor = ((DEFAULT_BLOCK_SIZE)*2)/ba->obj_size;
	}
	return a;
}

static void* block_obj_al_alloc(struct allocator *a,int32_t size)
{
	block_obj_allocator_t ba = (block_obj_allocator_t)a;
	struct thread_allocator *ta = (struct thread_allocator*)pthread_getspecific(ba->t_key);
	if(!ta)
	{
		ta = create_thread_allocator(ba);
		pthread_setspecific(ba->t_key,(void*)ta);
	}
	return thread_allocator_alloc(ta);
}

static void  block_obj_al_dealloc(struct allocator*a, void *ptr)
{
	block_obj_allocator_t ba = (block_obj_allocator_t)a;
	struct thread_allocator *ta = (struct thread_allocator*)pthread_getspecific(ba->t_key);
	assert(ta);
	thread_allocator_dealloc(ta,ptr);
}
	
static void destroy_block_obj_al(struct allocator **a)
{
	block_obj_allocator_t ba = (block_obj_allocator_t)*a;
    //销毁所有的thread_cache
    {
		list_node *l = link_list_head(ba->_thread_allocators);
		while(l)
		{
			struct thread_allocator *ta = (struct thread_allocator*)l;
			l = l->next;
			destroy_thread_allocator(ta); 
		}
		LINK_LIST_DESTROY(&ba->_thread_allocators);
	}
	release_freelist(ba->_free_list);
	LINK_LIST_DESTROY(&ba->_free_list);
	spin_destroy(&(ba->mtx));
	pthread_key_delete(ba->t_key);
	free(ba);
	*a = NULL;	
}

extern uint8_t GetK(uint32_t size);
block_obj_allocator_t create_block_obj_allocator(uint32_t obj_size)
{
	if(obj_size < sizeof(void*))
		obj_size = sizeof(void*);
	uint8_t k = GetK(obj_size);
	obj_size = 1 << k;
	block_obj_allocator_t ba = (block_obj_allocator_t)calloc(1,sizeof(*ba));
	ba->mtx = spin_create();
	ba->_thread_allocators = LINK_LIST_CREATE();
	ba->_free_list = LINK_LIST_CREATE();
	ba->obj_size = obj_size;
	pthread_key_create(&ba->t_key,0);
	ba->super_class.Alloc = block_obj_al_alloc;
	ba->super_class.DeAlloc = block_obj_al_dealloc;
	ba->super_class.Destroy = destroy_block_obj_al;
	return ba;
}


/*

struct free_list{
	list_node next;
	uint32_t  size;
	uint32_t  init_size;
	list_node *head;
	list_node *tail;
	void *mem;
};

struct thread_allocator
{
	list_node next;
	block_obj_allocator_t central_allocator;
	struct link_list *_free_list;
	struct link_list *_recover;
	uint32_t   free_size;
	uint16_t   array_idx;
	uint32_t   collect_factor;
};

struct thread_cache
{
	list_node next;
	struct thread_allocator _allocator[17];
};

struct block_obj_allocator
{
	IMPLEMEMT(allocator);
	pthread_key_t t_key;
	spinlock_t _free_list_mtx[17];
	struct link_list *_free_list[17];
	spinlock_t mtx;
	struct link_list *thread_caches;
};


static void *free_list_get(struct free_list *f)
{
	void *ptr = (void*)f->head;
	f->head = f->head->next;
	if(!f->head)
		f->tail = NULL;
	--f->size;	
	return ptr;
}

static void free_list_put(struct free_list *f,void *ptr)
{
	list_node *l = (list_node*)ptr;
	l->next = NULL;
	if(f->tail)
	{
		f->tail->next = l;
		f->tail = l;
	}
	else
		f->head = f->tail = l;
	++f->size;
}

#define DEFAULT_BLOCK_SIZE 1024*1024

static struct free_list *creat_new_freelist(uint32_t size)
{
	uint32_t init_size = DEFAULT_BLOCK_SIZE/size;
	struct free_list *f = (struct free_list*)calloc(1,sizeof(*f));
	assert(f);
	f->mem = calloc(1,DEFAULT_BLOCK_SIZE);
	assert(f->mem);
	f->init_size = f->size = init_size;
	int32_t i = 0;
	for( ; i < init_size; ++i)
	{
		list_node *l = (list_node*)(((uint8_t*)f->mem)+(i*size));
		free_list_put(f,l);
	}
	f->size = init_size;
	return f;
	
}

static struct free_list *central_get_freelist(block_obj_allocator_t central,uint16_t array_idx)
{
	//printf("central_get_freelist\n");
	struct free_list *f;
	spin_lock(central->_free_list_mtx[array_idx],4000);
	f = (struct free_list*)link_list_pop(central->_free_list[array_idx]);
	spin_unlock(central->_free_list_mtx[array_idx]);
	if(!f)
	{
		//printf("creat_new_freelist\n");
 	    f = creat_new_freelist(1<<array_idx);			
	}
	return f;
}

static void give_back_to_central(block_obj_allocator_t central,uint16_t array_idx,struct free_list *f)
{
	//printf("give_back_to_central\n");
	spin_lock(central->_free_list_mtx[array_idx],4000);
	LINK_LIST_PUSH_BACK(central->_free_list[array_idx],f);
	spin_unlock(central->_free_list_mtx[array_idx]);
}


void *thread_allocator_alloc(struct thread_allocator *a)
{
	void *ptr;
	struct free_list *f;
	if(!a->free_size)
	{
		//thread cache不够内存了，从central获取
		f = central_get_freelist(a->central_allocator,a->array_idx);
		assert(f);
		LINK_LIST_PUSH_BACK(a->_free_list,f);
		a->free_size += f->size;
	}
	else
	{
		f = (struct free_list*)link_list_head(a->_free_list);
		if(!f)
		{
			f = (struct free_list*)link_list_pop(a->_recover);
			LINK_LIST_PUSH_BACK(a->_free_list,f);
		}
	}
	ptr = free_list_get(f);
	assert(ptr);
	--a->free_size;
	if(!f->size)
	{
		link_list_pop(a->_free_list);
		link_list_push_back(a->_recover,(list_node*)f);
	}
	return ptr;
}

void thread_allocator_dealloc(struct thread_allocator *a,void *ptr)
{
	struct free_list *f = (struct free_list*)link_list_head(a->_recover);
	if(f)
	{
		free_list_put(f,ptr);
		++a->free_size;
		if(f->size == f->init_size)
		{
			link_list_pop(a->_recover);	
			//printf("==init_size\n");
			//一个free_list回收满了,要么放到free_list中，要么归还central
			if(a->free_size >= a->collect_factor)
			{
				//将f归还给central_allocator;	
				give_back_to_central(a->central_allocator,a->array_idx,f);
				a->free_size -= f->size;
			}
			else
				link_list_push_back(a->_free_list,(list_node*)f);
		}
	}
	else
	{
		f = (struct free_list*)link_list_head(a->_free_list);
		assert(f);
		free_list_put(f,ptr);
		++a->free_size;
	}
}

void thread_allocator_info(struct thread_allocator *a)
{
	printf("free_size:%d\n",a->free_size);
	{
		struct free_list *f = (struct free_list*)link_list_head(a->_free_list);
		while(f)
		{
			printf("f size%d\n",f->size);
			f = (struct free_list*)((list_node*)f)->next;
		}
	}
	{	
		struct free_list *f = (struct free_list*)link_list_head(a->_recover);
		while(f)
		{
			printf("f recover size%d\n",f->size);
			f = (struct free_list*)((list_node*)f)->next;
		}
	}
	
}


extern uint8_t GetK(uint32_t size);

static struct thread_cache* thread_cache_create(block_obj_allocator_t ba)
{
	struct thread_cache *tc = calloc(1,sizeof(*tc));
	int32_t i = 0;
	for( ; i < 17; ++i)
	{
		tc->_allocator[i].central_allocator = ba;
		tc->_allocator[i]._free_list = LINK_LIST_CREATE();
		tc->_allocator[i]._recover = LINK_LIST_CREATE();
		tc->_allocator[i].array_idx = i;
		tc->_allocator[i].collect_factor = ((DEFAULT_BLOCK_SIZE)*2)/(1<<i);
	}
	spin_lock(ba->mtx,4000);
	LINK_LIST_PUSH_BACK(ba->thread_caches,tc);
	spin_unlock(ba->mtx);
	return tc; 
}

static void release_freelist(struct link_list *flist)
{
	list_node *l = link_list_head(flist);
	while(l)
	{
		struct free_list *f = (struct free_list*)l;
		l = l->next;
		free(f->mem);
		free(f);
		//printf("destroy_freelist\n");
	}	
}

static void destroy_thread_cache(struct thread_cache *tc)
{
	int32_t i = 0;
	for(; i < 17; ++i)
	{
		release_freelist(tc->_allocator[i]._free_list);
		release_freelist(tc->_allocator[i]._recover);
		LINK_LIST_DESTROY(&(tc->_allocator[i]._free_list));
		LINK_LIST_DESTROY(&(tc->_allocator[i]._recover));
	}
	free(tc);
}

static void* thread_cache_alloc(struct thread_cache *tc,uint32_t size)
{
	size += sizeof(int32_t);
	uint8_t k = GetK(size);
	size = 1 << k;
	int32_t *ptr = (int32_t*)thread_allocator_alloc(&(tc->_allocator[k]));
	*ptr = k;
	ptr++;
	return (void*)ptr;
}

static void  thread_cache_dealloc(struct thread_cache *tc,void *ptr)
{
	int32_t *_ptr = ((int32_t*)ptr)-1;
	uint8_t k = *_ptr;
	thread_allocator_dealloc(&(tc->_allocator[k]),_ptr);
}

static void thread_cache_info(struct thread_cache *tc,uint32_t size)
{
	size += sizeof(int32_t);
	uint8_t k = GetK(size);
	thread_allocator_info(&(tc->_allocator[k]));
}

static void* block_obj_al_alloc(struct allocator *a, int32_t size)
{
	block_obj_allocator_t ba = (block_obj_allocator_t)a;
	struct thread_cache *tc = (struct thread_cache*)pthread_getspecific(ba->t_key);
	if(!tc)
	{
		tc = thread_cache_create(ba);
		pthread_setspecific(ba->t_key,(void*)tc);
	}
	return thread_cache_alloc(tc,size);
}

static void  block_obj_al_dealloc(struct allocator*a, void *ptr)
{
	block_obj_allocator_t ba = (block_obj_allocator_t)a;
	struct thread_cache *tc = (struct thread_cache*)pthread_getspecific(ba->t_key);
	assert(tc);
	thread_cache_dealloc(tc,ptr);
}
	
static void destroy_block_obj_al(struct allocator **a)
{
	block_obj_allocator_t ba = (block_obj_allocator_t)*a;
    //销毁所有的thread_cache
    {
		list_node *l = link_list_head(ba->thread_caches);
		while(l)
		{
			struct thread_cache *tc = (struct thread_cache *)l;
			l = l->next;
			destroy_thread_cache(tc); 
		}
		LINK_LIST_DESTROY(&ba->thread_caches);
	}
	//销毁所有free_list
	{
		int32_t i = 0;
		for( ; i < 17; ++i)
		{
			release_freelist(ba->_free_list[i]);
			LINK_LIST_DESTROY(&ba->_free_list[i]);
		}
	}
	{
		int32_t i = 0;
		for( ; i < 17; ++i)
		{
			spin_destroy(&(ba->_free_list_mtx[i]));
		}
	}
	spin_destroy(&(ba->mtx));
	pthread_key_delete(ba->t_key);
	free(ba);
	*a = NULL;	
}

block_obj_allocator_t create_block_obj_allocator()
{
	block_obj_allocator_t ba = (block_obj_allocator_t)calloc(1,sizeof(*ba));
	ba->mtx = spin_create();
	ba->thread_caches = LINK_LIST_CREATE();
	int32_t i = 0;
	for( ; i < 17; ++i)
	{
		ba->_free_list[i] = LINK_LIST_CREATE();
		ba->_free_list_mtx[i] = spin_create();
	}
	pthread_key_create(&ba->t_key,0);
	ba->super_class.Alloc = block_obj_al_alloc;
	ba->super_class.DeAlloc = block_obj_al_dealloc;
	ba->super_class.Destroy = destroy_block_obj_al;
	return ba;
}

void print_info(block_obj_allocator_t ba,int size)
{
	struct thread_cache *tc = (struct thread_cache*)pthread_getspecific(ba->t_key);
	thread_cache_info(tc,size);
}
*/
