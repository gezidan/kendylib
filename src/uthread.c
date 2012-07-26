#include <stdint.h>
#include "uthread.h"
#include <stdlib.h>
#include <stdio.h>

struct uthread
{
	//0:ebp,1:esp,2:ebx,3:edi,4:esi
	uint32_t reg[5];
	struct uthread *parent;//如果_parent非空,则_start_fun结束后返回到_parent中
	void *arg;
	uint32_t __init;
	void *stack;
	uint32_t stack_size;
};

uthread_t uthread_create(void *stack,uint32_t stack_size)
{
	uthread_t u = calloc(1,sizeof(*u));
	u->stack = stack;
	u->stack_size = stack_size;
	u->reg[0] = (uint32_t)stack+stack_size;
	u->reg[4] = (uint32_t)stack+stack_size;
	u->__init = 0;
	return u;
}

//汇编函数
extern void uthread_build_stack(uthread_t u);
extern void* uthread_switch(uthread_t from,uthread_t to,void *arg);
extern void uthread_start_run(uthread_t u,void* (*)(uthread_t,start_fun),start_fun);

void* uthread_swtch(uthread_t from,uthread_t to,void *arg)
{
	if(!to->__init)
		to->__init = 1;
	to->arg = arg;
	uthread_switch(from,to,arg);
}


void* uthread_run_main(uthread_t u,start_fun st_fun)
{
	if(!u->__init)
	{
		uthread_build_stack(u);
	}
	if(u->__init)
	{
		//执行st_fun
		void *ret = st_fun(u->arg);
		if(u->parent)
			uthread_swtch(u,u->parent,ret);
	}
}

void uthread_make(uthread_t u,uthread_t p,start_fun st_fun)
{
	u->parent = p;
	uthread_start_run(u,uthread_run_main,st_fun);
}

void uthread_destroy(uthread_t *u)
{
	free(*u);
	*u = 0;
}
