#include <stdio.h>
#include "cosche.h"
#include "SysTime.h"
void *test_coro_fun(void *arg)
{
	printf("test_coro_fun\n");
	int i = 0;
	uint32_t tick = GetSystemMs();
	for( ; i < 20000000; ++i)
	{
		//coro_sleep(get_current_coro(),500);
		coro_yield(get_current_coro());
		//printf("i back:%x\n",get_current_coro());
	}
	printf("%u\n",GetSystemMs()-tick);
}

int main()
{
	sche_t s = sche_create(100,4096);
	sche_spawn(s,test_coro_fun,NULL);
	//sche_spawn(s,test_coro_fun,NULL);
	//sche_spawn(s,test_coro_fun,NULL);
	//sche_spawn(s,test_coro_fun,NULL);	
	sche_schedule(s);
	return 0;
}
