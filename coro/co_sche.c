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
	printf("a coro start\n");
	coro_t co = (coro_t)arg;
	LINK_LIST_PUSH_BACK(co->_sche->active_list,co);
	co->status = CORO_ACTIVE;
	uthread_switch(co->ut,co->_sche->co->ut,co);
	void *ret = co->fun(co->arg);
	co->status = CORO_DIE;
	uthread_switch(co->ut,co->_sche->co->ut,co);
	return NULL;
}


static inline  __attribute__((always_inline)) void check_time_out(sche_t s,uint32_t now)
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
			LINK_LIST_PUSH_BACK(s->active_list,co);
		}else if(co->status == CORO_DIE)
		{
			coro_destroy(&co);
			if(--s->coro_size == 0)
				s->stop = 1;
			printf("a coro destroy\n");
		}	
	}
	s->next_check_timeout = now + 200;//check every 200 ms
}


static inline  __attribute__((always_inline))  coro_t _sche_next(sche_t s,coro_t co)
{
	coro_t next = LINK_LIST_POP(coro_t,s->active_list);
	if(!next)
		next = s->co;
	if(co->status == CORO_YIELD)
	{
		co->status = CORO_ACTIVE;
		LINK_LIST_PUSH_BACK(s->active_list,co);
	}
	assert(co != next);	
	set_current_coro(next);
	s->ti++;
	return (coro_t)uthread_switch(co->ut,next->ut,co);
}

static inline  __attribute__((always_inline)) void sche_next(sche_t s,coro_t co,uint8_t status)
{
	co->status = status;
	if(s->ti >= 100)
	{
		s->ti = 0;
		uint32_t tick = GetSystemMs();
		if(tick >= s->next_check_timeout)
			check_time_out(s,tick);
	}
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
	while(!s->stop)
	{
		
		if(s->ti >= 100)
		{
			s->ti = 0;
			uint32_t now = GetSystemMs();
			if(now >= s->next_check_timeout)
				check_time_out(s,now);
		}	
		if(link_list_is_empty(s->active_list))
		{
			printf("sleep\n");
			usleep(50);
			s->ti += 50;
		}
		else
		{
			coro_t co = _sche_next(s,s->co);
			if(co->status == CORO_DIE && co->_heapele.index == 0)
			{
				coro_destroy(&co);
				if(--s->coro_size == 0)
					s->stop = 1;
				printf("a coro destroy\n");
			}
		}
	}
	printf("schedule end\n");
}

sche_t sche_create(int32_t max_coro,int32_t stack_size)
{
	sche_t s = calloc(1,sizeof(*s));
	s->stack_size = stack_size;
	s->max_coro = max_coro;
	s->active_list = LINK_LIST_CREATE();
	s->_minheap = minheap_create(max_coro,_less);
	s->next_check_timeout = GetSystemMs() + 200;
	s->co = coro_create(s,0,NULL);
	set_current_coro(s->co);
	return s;
}

void sche_destroy(sche_t *s)
{
	coro_t co;
	while(co = LINK_LIST_POP(coro_t,(*s)->active_list))
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
	LINK_LIST_DESTROY(&((*s)->active_list));
	minheap_destroy(&((*s)->_minheap));
	coro_destroy(&(*s)->co);
	free(*s);
	*s = NULL;
}

struct coro *sche_spawn(sche_t s,void*(*fun)(void*),void*arg)
{
	coro_t co = coro_create(s,4096,coro_fun);
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
		co->stack = calloc(1,stack_size);
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
	co->timeout = GetSystemMs() + ms;
	sche_add_timeout(co->_sche,co);
}



