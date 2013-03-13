#include <stdio.h>
#include "co_sche.h"
#include "util/SysTime.h"
#include <stdlib.h>
void *test_coro_fun(void *arg)
{
	printf("test_coro_fun\n");
	int i = 0;
	uint32_t tick = GetSystemMs();
	coro_t co = get_current_coro();
	//printf("test_coro_fun\n");
	for( ; i < 30000000; ++i)
	{
		//printf("test_coro_fun\n");
		//coro_sleep(get_current_coro(),500);
		coro_yield(co);
		//printf("i back:%x\n",get_current_coro());
	}
	printf("%u\n",GetSystemMs()-tick);
}
#include <time.h>  
#include <sys/time.h> 

int main()
{
	sche_t s = sche_create(100,4096,NULL,NULL);
	sche_spawn(s,test_coro_fun,NULL);
	
	//sche_spawn(s,test_coro_fun,NULL);
	//sche_spawn(s,test_coro_fun,NULL);
	//sche_spawn(s,test_coro_fun,NULL);
	while(1)
	sche_schedule(s);
	printf("end\n");
	/*int i = 0;
	uint32_t tick = GetSystemMs();
	//struct timespec now;
	struct timeval stick0;
	for( ; i < 10000000; ++i)
	{
		//gettimeofday(&stick0, NULL); 
		GetSystemMs();	
		//clock_gettime(CLOCK_MONOTONIC, &now);
	}
	printf("%u\n",GetSystemMs()-tick);	
	*/
	return 0;
}
