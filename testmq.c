#include <stdio.h>
#include <stdlib.h>
#include "KendyNet.h"
#include "thread.h"
#include "SocketWrapper.h"
#include "atomic.h"
#include "SysTime.h"
#include "mq.h"

list_node *node_list1[5];
list_node *node_list2[5];
mq_t mq1;

void *Routine1(void *arg)
{
	int j = 0;
	for( ; ; )
	{
		int i = 0;
		for(; i < 10000000; ++i)
		{
			mq_push(mq1,&node_list1[j][i]);
		}
		mq_force_sync(mq1);
		j = (j + 1)%5; 
		sleepms(100);
		
	}
}

void *Routine3(void *arg)
{
	int j = 0;
	for( ; ; )
	{
		int i = 0;
		for(; i < 10000000; ++i)
		{
			mq_push(mq1,&node_list2[j][i]);
		}
		mq_force_sync(mq1);
		j = (j + 1)%5; 
		sleepms(100);
		
	}
}

void *Routine2(void *arg)
{
	uint64_t count = 0;
	uint32_t tick = GetCurrentMs();
	for( ; ; )
	{
		list_node *n = mq_pop(mq1,50);
		if(n)
		{
			++count;
		}
		uint32_t now = GetCurrentMs();
		if(now - tick > 1000)
		{
			printf("recv:%d\n",(count*1000)/(now-tick));
			tick = now;
			count = 0;
		}
	}
}


int main()
{
	int i = 0;
	for( ; i < 5; ++i)
	{
		node_list1[i] = calloc(10000000,sizeof(list_node));
		node_list2[i] = calloc(10000000,sizeof(list_node));
	}
	mq1 = create_mq(4096);
	init_system_time(10);
	thread_t t1 = create_thread(0);
	start_run(t1,Routine1,NULL);
	
	thread_t t3 = create_thread(0);
	start_run(t3,Routine3,NULL);	

	thread_t t2 = create_thread(0);
	start_run(t2,Routine2,NULL);

	getchar();
	
	return 0;
}

