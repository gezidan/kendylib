#include "coro.h"
#include <stdlib.h>
#include <time.h>

void* yield(struct coro *from,struct coro *to,void *arg)
{
	return uthread_swtch(from->ut,to->ut,arg);
}

void* resume(struct coro *from,struct coro *to,void *arg)
{
	return uthread_swtch(from->ut,to->ut,arg);
}

static uint32_t g_index = 0;


static void* coro_start_fun(void *arg)
{
	struct testarg *_arg = (struct testarg *)arg;
	void *ret = _arg->co->st_fun(_arg);
	_arg->co->is_end = 1;
	return ret;
}


void spawn(struct scheduler *sche,void *stack,uint32_t stack_size,start_fun st_fun)
{
	uthread_t ut = uthread_create(stack,stack_size);
	struct coro *co = (struct coro*)malloc(sizeof(*co));
	co->ut = ut;
	co->st_fun = st_fun;
	co->id = ++g_index;
	//添加到激活队列中
	co->next = sche->active;
	co->is_end = 0;
	sche->active = co;
	uthread_make(co->ut,sche->self->ut,coro_start_fun);
}

struct scheduler *scheduler_create()
{
	struct scheduler *sche = (struct scheduler *)malloc(sizeof(*sche));
	sche->active = 0;
	sche->self = (struct coro*)malloc(sizeof(*sche->self));
	sche->self->ut = uthread_create(0,0);
	return sche;
}

void schedule(struct scheduler *sche)
{
	while(1)
	{
		if(sche->active)
		{
			struct coro *cur = sche->active;
			sche->active = 0;
			while(cur)
			{	
				struct testarg arg = {sche->self,cur};
				resume(sche->self,cur,&arg);
				struct coro *tmp = cur->next;
				if(!cur->is_end)
				{	
					cur->next = sche->active;
					sche->active = cur;
					cur = tmp;
				}
				else
				{
					uthread_destroy(&(cur->ut));
					free(cur);
				}
				cur = tmp;
			}
		}
		else
			break;
	}

}