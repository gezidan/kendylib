#include "co_sche.h"
#include "SysTime.h"
#include <stdlib.h>
#include <assert.h>
static coro_t current_coro = NULL;

static inline int8_t _less(struct heapele*l,struct heapele*r)
{
	return ((coro_t)l)->timeout < ((coro_t)r)->timeout;
}

static inline  __attribute__((always_inline))void set_current_coro(coro_t co)
{
	current_coro = co;
}

void* coro_fun(void *arg)
{
	//printf("a coro start\n");
	coro_t co = (coro_t)arg;
	LINK_LIST_PUSH_BACK(co->_sche->active_list_2,co);
	co->status = CORO_ACTIVE;
	uthread_switch(co->ut,co->_sche->co->ut,co);
	void *ret = co->fun(co->arg);
	co->status = CORO_DIE;
	uthread_switch(co->ut,co->_sche->co->ut,co);
	return NULL;
}


void check_time_out(sche_t s,uint32_t now)
{
	coro_t co;
	for( ; ;)
	{
		struct heapele *ele = minheap_min(s->_minheap);
		if(!ele)
			break;
		co = (coro_t)((int8_t*)ele - sizeof(co->next));
		if(co->timeout < now)
			break;
		minheap_popmin(s->_minheap);
		if(co->status != CORO_ACTIVE)
		{
			co->status = CORO_ACTIVE;
			LINK_LIST_PUSH_BACK(s->active_list_1,co);
		}else if(co->status == CORO_DIE)
		{
			coro_destroy(&co);
			if(--s->coro_size == 0)
				s->stop = 1;
			//printf("a coro destroy\n");
		}	
	}
	s->next_check_timeout = now + 200;//check every 200 ms
}


/*
*  next选择方案,如果有goback优先选择goback作为下一个被运行的coro,
*  否则取active_list_1首元素作为下一个被运行的coro,如果active_list_1
*  为空则直接返回.
*/
coro_t _sche_next_1(sche_t s,coro_t co)
{

	coro_t next = NULL;
	coro_t coro_goback = co->_goback;
	if(NULL == coro_goback)
	{	
		coro_t next = LINK_LIST_POP(coro_t,s->active_list_1);
		if(!next)
			return NULL;
	}
	else
	{
		next = coro_goback;
		co->_goback = NULL;
	}
	LINK_LIST_PUSH_BACK(s->active_list_2,co);
	assert(co != next);	
	set_current_coro(next);
	return (coro_t)uthread_switch(co->ut,next->ut,co);	
}

/*
*  next选择方案,如果有goback优先选择goback作为下一个被运行的coro,
*  否则取active_list_1首元素作为下一个被运行的coro,如果active_list_1
*  为空,取active_list_2首元素作为下一个被运行的coro.如果active_list_2
*  也为空,返回到主调度器所在的coro中
*/

static inline  __attribute__((always_inline))  coro_t _sche_next(sche_t s,coro_t co)
{

	coro_t next = NULL;
	coro_t coro_goback = co->_goback;
	if(NULL == coro_goback)
	{
		next = LINK_LIST_POP(coro_t,s->active_list_1);
		if(!next)
			next = LINK_LIST_POP(coro_t,s->active_list_2);
		if(!next)	
			next = s->co;
	}
	else
	{
		next = coro_goback;
		co->_goback = NULL;
	}
	
	if(co->status == CORO_YIELD)
	{
		co->status = CORO_ACTIVE;
		LINK_LIST_PUSH_BACK(s->active_list_2,co);
	}
	assert(co != next);
	set_current_coro(next);
	return (coro_t)uthread_switch(co->ut,next->ut,co);
}

static inline  __attribute__((always_inline)) void sche_next(sche_t s,coro_t co,uint8_t status)
{
	co->status = status;

	uint32_t tick = GetCurrentMs();
	if(tick >= s->next_check_timeout)
		check_time_out(s,tick);
	_sche_next(s,co);
}

static inline  __attribute__((always_inline)) void sche_add_timeout(sche_t s,coro_t co)
{
	co->status = CORO_SLEEP;
	struct heapele *hele = &(co->_heapele);
	minheap_insert(s->_minheap,hele);
	_sche_next(s,co);
}

void sche_schedule(sche_t s)
{
	uint32_t now = GetCurrentMs();
	if(now >= s->next_check_timeout)
		check_time_out(s,now);

	if(link_list_is_empty(s->active_list_1) && link_list_is_empty(s->active_list_2))
	{
		if(s->idel)
			s->idel(s->idel_arg);
		else
			sleepms(50);
	}
	else
	{
		coro_t co = _sche_next(s,s->co);
		if(co->status == CORO_DIE && co->_heapele.index == 0)
		{
			coro_destroy(&co);
			printf("a coro destroy\n");
		}
	}
}

sche_t sche_create(int32_t max_coro,int32_t stack_size,void (*idel)(void*),void *idel_arg)
{
	init_system_time(10);
	sche_t s = calloc(1,sizeof(*s));
	s->stack_size = stack_size;
	s->max_coro = max_coro;
	s->active_list_1 = LINK_LIST_CREATE();
	s->active_list_2 = LINK_LIST_CREATE();
	s->_minheap = minheap_create(max_coro,_less);
	s->next_check_timeout = GetSystemMs() + 200;
	s->co = coro_create(s,0,NULL);
	s->idel = idel;
	s->idel_arg = idel_arg;
	set_current_coro(s->co);
	return s;
}

void sche_destroy(sche_t *s)
{
	coro_t co;
	while(co = LINK_LIST_POP(coro_t,(*s)->active_list_1))
		coro_destroy(&co);
	while(co = LINK_LIST_POP(coro_t,(*s)->active_list_2))
		coro_destroy(&co);		
	int32_t i = 1;
	for( ; i < (*s)->_minheap->size; ++i)
	{
		if((*s)->_minheap->data[i])
		{
			(coro_t)((int8_t*)(*s)->_minheap->data[i] - sizeof(co->next));
			coro_destroy(&co);
		}
	}
	LINK_LIST_DESTROY(&((*s)->active_list_1));
	LINK_LIST_DESTROY(&((*s)->active_list_2));
	minheap_destroy(&((*s)->_minheap));
	coro_destroy(&(*s)->co);
	free(*s);
	*s = NULL;
}

struct coro *sche_spawn(sche_t s,void*(*fun)(void*),void*arg)
{
	if(s->coro_size >= s->max_coro)
		return NULL;
	coro_t co = coro_create(s,s->stack_size,coro_fun);
	co->arg = arg;
	co->fun = fun;
	++s->coro_size;
	uthread_switch(s->co->ut,co->ut,co);
	return co;
}

coro_t coro_create(struct sche *_sche,uint32_t stack_size,void*(*fun)(void*))
{
	coro_t co = calloc(1,sizeof(*co));
	co->_sche = _sche;
	if(stack_size)
	{
		co->stack = calloc(1,stack_size);
		if(!co->stack)
		{
			free(co);
			return NULL;
		}
	}
	co->ut = uthread_create(NULL,co->stack,stack_size,fun);
	return co;
}

void coro_destroy(coro_t *co)
{
	if((*co)->stack);
		free((*co)->stack);
	uthread_destroy(&((*co)->ut));
	free(*co);
	*co = NULL;
}

coro_t get_current_coro()
{
	return current_coro;
}

void coro_yield(coro_t co)
{
	sche_next(co->_sche,co,CORO_YIELD);
}

void coro_sleep(coro_t co,int32_t ms)
{
	co->timeout = GetCurrentMs() + ms;
	sche_add_timeout(co->_sche,co);
}

void coro_block(coro_t co)
{
	sche_next(co->_sche,co,CORO_BLOCK);
}

void coro_wakeup(coro_t co)
{
	if(co->status == CORO_BLOCK)
		LINK_LIST_PUSH_BACK(co->_sche->active_list_1,co);
}



