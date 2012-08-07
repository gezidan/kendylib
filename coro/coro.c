#include "coro.h"
#include "cosche.h"
#include <stdlib.h>
#include "SysTime.h"
#include "uthread.h"
coro_t current_coro = NULL; 

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
	free((*co)->stack);
	uthread_destroy(&((*co)->ut));
	free(*co);
	*co = NULL;
}

coro_t get_current_coro()
{
	return current_coro;
}

void set_current_coro(coro_t co)
{
	current_coro = co;
}

extern void sche_next(sche_t,coro_t co,uint8_t status);
extern void sche_add_timeout(sche_t,coro_t co);

void coro_yield(coro_t co)
{
	sche_next(co->_sche,co,CORO_YIELD);
}

void coro_sleep(coro_t co,int32_t ms)
{
	co->timeout = GetSystemMs() + ms;
	sche_add_timeout(co->_sche,co);
}




