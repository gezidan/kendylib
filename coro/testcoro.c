#include <stdio.h>
#include "cosche.h"
#include "coro.h"

void *test_coro_fun(void *arg)
{
	printf("test_coro_fun\n");
	int i = 0;
	for( ; i < 10; ++i)
	{
		coro_sleep(get_current_coro(),500);
		printf("i back:%x\n",get_current_coro());
	}
}

int main()
{
	sche_t s = sche_create(100,4096);
	sche_spawn(s,test_coro_fun,NULL);
	sche_spawn(s,test_coro_fun,NULL);
	sche_spawn(s,test_coro_fun,NULL);
	sche_spawn(s,test_coro_fun,NULL);	
	sche_schedule(s);
	return 0;
}
