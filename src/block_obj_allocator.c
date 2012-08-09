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

static inline struct free_list *central_get_freelist(block_obj_allocator_t central)
{
	//printf("central_get_freelist\n");
	struct free_list *f;
	if(central->mtx)
	{
		spin_lock(central->mtx);
		f = (struct free_list*)link_list_pop(central->_free_list);
		spin_unlock(central->mtx);
	}
	else
		f = (struct free_list*)link_list_pop(central->_free_list);
	if(!f)
	{
		//printf("creat_new_freelist\n");
 	    f = creat_new_freelist(central->obj_size);			
	}
	return f;
}

static inline void give_back_to_central(block_obj_allocator_t central,struct free_list *f)
{
	//printf("give_back_to_central\n");
	if(central->mtx)
	{
		spin_lock(central->mtx);
		LINK_LIST_PUSH_BACK(central->_free_list,f);
		spin_unlock(central->mtx);
	}
	else
		LINK_LIST_PUSH_BACK(central->_free_list,f);
}

static inline void *thread_allocator_alloc(struct thread_allocator *a)
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

static inline void thread_allocator_dealloc(struct thread_allocator *a,void *ptr)
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
		LINK_LIST_PUSH_BACK(ba->_thread_allocators,a);
	}
	return a;
}

static void* block_obj_al_alloc(struct allocator *a,int32_t size)
{
	block_obj_allocator_t ba = (block_obj_allocator_t)a;
	struct thread_allocator *ta;
	if(ba->mtx)
	{
		ta = (struct thread_allocator*)pthread_getspecific(ba->t_key);
		if(!ta)
		{
			ta = create_thread_allocator(ba);
			pthread_setspecific(ba->t_key,(void*)ta);
		}
	}
	else
	{
		ta = (struct thread_allocator*)link_list_head(ba->_thread_allocators);
		if(!ta)
			ta = create_thread_allocator(ba);
	}
	return thread_allocator_alloc(ta);
}

static void  block_obj_al_dealloc(struct allocator*a, void *ptr)
{
	block_obj_allocator_t ba = (block_obj_allocator_t)a;
	struct thread_allocator *ta;
	if(ba->mtx)
		ta = (struct thread_allocator*)pthread_getspecific(ba->t_key);
	else
		ta = (struct thread_allocator*)link_list_head(ba->_thread_allocators);
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
	if(ba->mtx)
	{
		spin_destroy(&(ba->mtx));
		pthread_key_delete(ba->t_key);
	}
	free(ba);
	*a = NULL;	
}

extern uint8_t GetK(uint32_t size);
block_obj_allocator_t create_block_obj_allocator(uint8_t mt,uint32_t obj_size)
{
	if(obj_size < sizeof(void*))
		obj_size = sizeof(void*);
	uint8_t k = GetK(obj_size);
	obj_size = 1 << k;
	block_obj_allocator_t ba = (block_obj_allocator_t)calloc(1,sizeof(*ba));
	if(mt)
	{
		ba->mtx = spin_create();
		pthread_key_create(&ba->t_key,0);
	}
	ba->_thread_allocators = LINK_LIST_CREATE();
	ba->_free_list = LINK_LIST_CREATE();
	ba->obj_size = obj_size;
	ba->super_class.Alloc = block_obj_al_alloc;
	ba->super_class.DeAlloc = block_obj_al_dealloc;
	ba->super_class.Destroy = destroy_block_obj_al;
	return ba;
}

