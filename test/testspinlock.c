#include "util/thread.h"
#include "util/atomic.h"
#include "util/spinlock.h"
#include <stdio.h>
#include "util/SysTime.h"

spinlock_t g_lock;
static atomic_32_t counter = 0;

void *thread_routine1(void *arg)
{
	int32_t i = 0;
	for( ;i < 10000000; ++i)
	{
		spin_lock(g_lock);
		++counter;
		spin_unlock(g_lock);
	}
	return NULL;
}

void *thread_routine2(void *arg)
{
	int32_t i = 0;
	for( ;i < 10000000; ++i)
	{
		spin_lock(g_lock);
		++counter;
		spin_unlock(g_lock);
	}
	return NULL;
}

int main()
{
	g_lock = spin_create();
	int i = 0; 
	for(; i < 10; ++i)
	{
		counter = 0;
		thread_t t1 = create_thread(1);
		thread_t t2 = create_thread(1);
		thread_t t3 = create_thread(1);
		thread_t t4 = create_thread(1);		
		uint32_t tick = GetSystemMs();
		thread_start_run(t1,thread_routine1,NULL);
		thread_start_run(t2,thread_routine2,NULL);
		thread_start_run(t3,thread_routine1,NULL);
		thread_start_run(t4,thread_routine2,NULL);		
		thread_join(t1);
		thread_join(t2);
		thread_join(t3);
		thread_join(t4);		
		printf("%u,%d\n",GetSystemMs()-tick,counter);
		destroy_thread(&t1);
		destroy_thread(&t2);
	}
	return 0;
}
