#include <stdio.h>
#include "uthread.h"
#include "SysTime.h"
#include <stdlib.h>
void* ufun2(void *arg)
{
	printf("ufun2\n");
	char **tmp = (char**)arg;
	uthread_t self = (uthread_t)tmp[0];
	uthread_t parent = (uthread_t)tmp[1];
	volatile void *ptr = self;
	while(ptr)
	{
		ptr = uthread_switch(self,parent,NULL);
	}
	return NULL;
}

char *stack1;
char *stack2;

void* ufun1(void *arg)
{
	uthread_t self = (uthread_t)arg;
	uthread_t u = uthread_create(self,stack2,4096,ufun2);
	char* _arg[2];
	_arg[0] = (char*)u;
	_arg[1] = (char*)self;
	int i = 0;
	uint32_t tick = GetSystemMs();
	for( ; i < 30000000; ++i)
	{
		uthread_switch(self,u,&_arg[0]);
	}
	printf("%d\n",GetSystemMs()-tick);
	uthread_switch(self,u,NULL);
	return arg;
}

int main()
{
	stack1 = (char*)malloc(4096);
	stack2 = (char*)malloc(4096);
	/*
	if use ucontext version
	char dummy_stack[4096];
	uthread_t p = uthread_create(NULL,dummy_stack,0,NULL);
	*/
	uthread_t p = uthread_create(NULL,NULL,0,NULL);
	uthread_t u = uthread_create(p,stack1,4096,ufun1);
	uthread_switch(p,u,u);
	printf("main end\n");
	return 0;
};
