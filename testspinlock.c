#include "thread.h"
#include "atomic.h"
#include "spinlock.h"
#include <stdio.h>


spinlock_t g_lock;
static atomic_32_t counter = 0;

void *thread_routine1(void *arg)
{
	int32_t i = 0;
	for( ;i < 5000000; ++i)
	{
		spin_lock(g_lock,0);
		++counter;
		spin_unlock(g_lock);
	}
	return NULL;
}

void *thread_routine2(void *arg)
{
	int32_t i = 0;
	for( ;i < 5000000; ++i)
	{
		spin_lock(g_lock,0);
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
		thread_start_run(t1,thread_routine1,NULL);
		thread_start_run(t2,thread_routine2,NULL);
		thread_join(t1);
		thread_join(t2);
		printf("%d\n",counter);
		destroy_thread(&t1);
		destroy_thread(&t2);
	}
	return 0;
}
