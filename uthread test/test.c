#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "uthread.h"
#include "coro.h"

void* fun(void *arg)
{
	struct testarg *_arg = (struct testarg *)arg;
	int i = 0;
	while(i<10)
	{
	   printf("%d\n",_arg->co->id);
	   yield(_arg->co,_arg->sche,0);
	   ++i;
	}
	return 0;
}

int main()
{
	struct scheduler *sche = scheduler_create();
	spawn(sche,malloc(4096),4096,fun);
	spawn(sche,malloc(4096),4096,fun);
	spawn(sche,malloc(4096),4096,fun);
	spawn(sche,malloc(4096),4096,fun);
	schedule(sche);
	return 0;
}

