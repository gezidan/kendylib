#include <stdio.h>
#include "util/uthread.h"
#include "util/SysTime.h"
#include <stdlib.h>

/*
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
	printf("ufun2 end\n");
	return NULL;
}

char *stack1;
char *stack2;

void* ufun1(void *arg)
{
	uthread_t self = (uthread_t)arg;
	uthread_t u = uthread_create(self,stack2,65536,ufun2);
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
	printf("ufun1 end\n");
	return arg;
}

int main()
{
	stack1 = (char*)malloc(65536);
	stack2 = (char*)malloc(65536);
	uthread_t p = uthread_create(NULL,NULL,0,NULL);
	uthread_t u = uthread_create(p,stack1,65536,ufun1);
	uthread_switch(p,u,u);
	printf("main end\n");
	return 0;
};
*/
float timeit(void (*func)(long))
{
    long count, elapsed;
    struct timeval start, end;

    count = 10;
    while (1) {
        gettimeofday(&start, NULL);
        func(count);
        gettimeofday(&end, NULL);
        elapsed = end.tv_usec - start.tv_usec;
        elapsed += 1000000 * (end.tv_sec - start.tv_sec);
        if (elapsed > 1000000)
            break;
        count *= 2;
    }
    return 1000000.0 * count / elapsed;
}

volatile long counter;
uthread_t gr1, gr2;
char *stack1;
char *stack2;

void *_greenlet_func1(void *arg)
{
    while (counter > 0)
        uthread_switch(gr1,gr2, NULL);
    return NULL;
}

void *_greenlet_func2(void *arg)
{
    while (1) {
        counter--;
        uthread_switch(gr2,gr1, NULL);
    }
    return NULL;
}

void time_greenlet_switch_to(long count)
{
	uthread_t p = uthread_create(NULL,NULL,0,NULL);
	gr2 = uthread_create(NULL,stack2,65536,_greenlet_func2);
	gr1 = uthread_create(NULL,stack1,65536,_greenlet_func1);
    counter = count;
    uthread_switch(p,gr1,NULL);
    uthread_destroy(&p);
    uthread_destroy(&gr1);
    uthread_destroy(&gr2);
}

int main(int argc, char **argv)
{
    stack1 = (char*)malloc(65536);
	stack2 = (char*)malloc(65536);
    float mps;
    mps = timeit(time_greenlet_switch_to) / 1000000.0;
    printf("greenlet_switch_to: %.2f million context switches/sec\n", mps);
    return 0;
}
