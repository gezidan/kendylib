#include <stdio.h>
#include "uthread.h"
#include "SysTime.h"
void* ufun2(void *arg)
{
	char **tmp = (char**)arg;
	uthread_t self = (uthread_t)tmp[0];
	uthread_t parent = (uthread_t)tmp[1];
	void *ptr = self;
	while(ptr)
	{
		ptr = uthread_switch(self,parent,NULL);
	}
}

void* ufun1(void *arg)
{
	uthread_t self = (uthread_t)arg;
	char stack[4096];
	uthread_t u = uthread_create(self,stack,4096,ufun2);
	char* _arg[2];
	_arg[0] = (char*)u;
	_arg[1] = (char*)self;
	int i = 0;
	uint32_t tick = GetSystemMs();
	for( ; i < 5000000; ++i)
		uthread_switch(self,u,&_arg[0]);
	printf("%d\n",GetSystemMs()-tick);
	uthread_switch(self,u,NULL);
	
	return arg;
}

int main()
{
	char stack[4096];
	char stack2[1];
	uthread_t p = uthread_create(NULL,stack2,0,NULL);
	uthread_t u = uthread_create(p,stack,4096,ufun1);
	printf("%x,%x\n",u,uthread_switch(p,u,u));
	printf("main end\n");
	return 0;
};
