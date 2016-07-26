#include "util/thread.h"
#include <stdio.h>
#include "util/sync.h"


void *routine(void *arg)
{
	barrior_t b =  (barrior_t)arg;
	int i = 0;
	for( ;i < 1000; ++i)
		printf("hello\n");
	barrior_wait(b);
	return 0;
}

int main()
{
	//thread_run(routine,0);	
	//getchar();
	//thread_t t = CREATE_THREAD_RUN(1,routine,0);
	//join(t);
	barrior_t b = barrior_create(10);
	int i = 0;
	for(i;i < 10; ++i)
		CREATE_THREAD_RUN(1,routine,(void*)b);
	getchar();
	barrior_destroy(&b);
	return 0;
}