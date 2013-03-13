#include "uthread.h"
#include <stdlib.h>
#include <pthread.h>
#include "link_list.h"

struct uthread
{
	int32_t reg[8];//0:esp,1:ebp,2:eax,3:ebx,4:ecx,5:edx,6:edi,7:esi
	void *para;
	uthread_t parent;
	void*(*main_fun)(void*);
	void *stack;
	int32_t ssize;
	int8_t first_run;
};

#ifdef _DEBUG
//for debug version
void uthread_main_function()
{
	int32_t arg;
	 __asm__ volatile(
		"movl %%eax,%0\t\n"
		:
		:"m"(arg)
	);	
	
	uthread_t u = (uthread_t)arg;
	void *ret = u->main_fun(u->para);
	if(u->parent)
		uthread_switch(u,u->parent,ret);
	else
		exit(0); 
}
#else
//for release version
void __attribute__((regparm(1))) uthread_main_function(void *arg)
{
	uthread_t u = (uthread_t)arg;
	void *ret = u->main_fun(u->para);
	if(u->parent)
		uthread_switch(u,u->parent,ret);
	else
		exit(0);
}
#endif
uthread_t uthread_create(uthread_t parent,void*stack,uint32_t stack_size,void*(*fun)(void*))
{
	uthread_t u = (uthread_t)calloc(1,sizeof(*u));
	u->parent = parent;
	u->main_fun = fun;
	u->stack = stack;
	u->ssize = stack_size;
	if(stack)
	{
		u->reg[0] = (int32_t)stack+stack_size-4;
		u->reg[1] = (int32_t)stack+stack_size-4;
	}
	if(u->main_fun)
		u->first_run = 1;
	return u;
}

void uthread_destroy(uthread_t *u)
{
	free(*u);
	*u = NULL;
}

#ifdef _DEBUG
void* __attribute__((regparm(3))) uthread_switch(uthread_t from,uthread_t to,void *para)
{
	if(!from)
		return NULL;
	to->para = para;
	int32_t esp,ebp,eax,ebx,ecx,edx,edi,esi;
	//save current registers
	//the order is important	
	 __asm__ volatile(
		"movl %%eax,%2\t\n"
		"movl %%ebx,%3\t\n"
		"movl %%ecx,%4\t\n"
		"movl %%edx,%5\t\n"
		"movl %%edi,%6\t\n"
		"movl %%esi,%7\t\n"	 
		"movl %%ebp,%1\t\n"
		"movl %%esp,%0\t\n"
		:
		:"m"(esp),"m"(ebp),"m"(eax),"m"(ebx),"m"(ecx),"m"(edx),"m"(edi),"m"(esi)
	);
	from->reg[0] = esp;
	from->reg[1] = ebp;
	from->reg[2] = eax;
	from->reg[3] = ebx;
	from->reg[4] = ecx;
	from->reg[5] = edx;
	from->reg[6] = edi;
	from->reg[7] = esi;	
	if(to->first_run)
	{
	   to->first_run = 0;
	   esp = to->reg[0];
	   //use eax to pass arg
	   eax = (int32_t)to;
		__asm__ volatile (
			"movl %1,%%eax\t\n"
			"movl %0,%%ebp\t\n"
			"movl %%ebp,%%esp\t\n"
			:
			:"m"(esp),"m"(eax)
		);	   
	   uthread_main_function();
	}
	else
	{
		esp = to->reg[0];
		ebp = to->reg[1];
		eax = to->reg[2];
		ebx = to->reg[3];
		ecx = to->reg[4];
		edx = to->reg[5];
		edi = to->reg[6];
		esi = to->reg[7];
		//the order is important
		__asm__ volatile (
			"movl %2,%%eax\t\n"
			"movl %3,%%ebx\t\n"
			"movl %4,%%ecx\t\n"
			"movl %5,%%edx\t\n"
			"movl %6,%%edi\t\n"
			"movl %7,%%esi\t\n"		
			"movl %1,%%ebp\t\n"
			"movl %0,%%esp\t\n"
			:
			:"m"(esp),"m"(ebp),"m"(eax),"m"(ebx),"m"(ecx),"m"(edx),"m"(edi),"m"(esi)
		);
	}	
	return from->para;
}
#else
void* __attribute__((regparm(3))) uthread_switch(uthread_t from,uthread_t to,void *para)
{
	if(!from)
		return NULL;
	to->para = para;
	int32_t esp,ebp,edi,esi;
	//save current registers
	//the order is important	
	 __asm__ volatile(
		"movl %%eax,%2\t\n"
		"movl %%ebx,%3\t\n"
		"movl %%ecx,%4\t\n"
		"movl %%edx,%5\t\n"
		"movl %%edi,%6\t\n"
		"movl %%esi,%7\t\n"	 
		"movl %%ebp,%1\t\n"
		"movl %%esp,%0\t\n"
		:
		:"m"(from->reg[0]),"m"(from->reg[1]),"m"(from->reg[2]),"m"(from->reg[3])
		,"m"(from->reg[4]),"m"(from->reg[5]),"m"(from->reg[6]),"m"(from->reg[7])
	);
	if(to->first_run)
	{
	   to->first_run = 0;   
	   //change stack
	   //the order is important
		__asm__ volatile (
			"movl %0,%%ebp\t\n"
			"movl %%ebp,%%esp\t\n"
			:
			:"m"(to->reg[0])
		);			   
	   uthread_main_function((void*)to);
	}
	else
	{
		esp = to->reg[0];
		ebp = to->reg[1];
		edi = to->reg[6];
		esi = to->reg[7];
		//the order is important
		__asm__ volatile (
			"movl %2,%%eax\t\n"
			"movl %3,%%ebx\t\n"
			"movl %4,%%ecx\t\n"
			"movl %5,%%edx\t\n"
			"movl %6,%%edi\t\n"
			"movl %7,%%esi\t\n"		
			"movl %1,%%ebp\t\n"
			"movl %0,%%esp\t\n"
			:
			:"m"(esp),"m"(ebp),"m"(to->reg[2]),"m"(to->reg[3])
			,"m"(to->reg[4]),"m"(to->reg[5]),"m"(edi),"m"(esi)
		);
	}	
	return from->para;
}
#endif
/*
#include <ucontext.h>
struct uthread
{
   ucontext_t ucontext;	
   void *para;   		
   uthread_t  parent;
   void* (*main_fun)(void*);
};

void uthread_main_function(void *arg)
{
	uthread_t u = (uthread_t)arg;
	void *ret = u->main_fun(u->para);
	if(u->parent)
		uthread_switch(u,u->parent,ret);
}

uthread_t uthread_create(uthread_t parent,void*stack,uint32_t stack_size,void*(*fun)(void*))
{
	uthread_t u = (uthread_t)calloc(1,sizeof(*u));
	u->ucontext.uc_stack.ss_sp = stack;
	u->ucontext.uc_stack.ss_size = stack_size;
	u->ucontext.uc_link = NULL;
	u->parent = parent;
	u->main_fun = fun;
	getcontext(&(u->ucontext));
	makecontext(&(u->ucontext),(void(*)())uthread_main_function,1,u);
	return u;
}

void uthread_destroy(uthread_t *u)
{
	
}

void*uthread_switch(uthread_t from,uthread_t to,void *para)
{
	if(!from)
		return NULL;
	to->para = para;
	swapcontext(&(from->ucontext),&(to->ucontext));
	return from->para;
}
*/
