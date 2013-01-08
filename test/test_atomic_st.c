#include "util/thread.h"
#include "util/SysTime.h"
#include "util/atomic.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util/sync.h"
#include "util/atomic_st.h"

struct point
{
	struct atomic_st base;
	volatile int x;
	volatile int y;
	volatile int z;
};

GET_ATOMIC_ST(GetPoint,struct point);	
SET_ATOMIC_ST(SetPoint,struct point);	
	
struct atomic_type *g_points[1000];

void *SetRotine(void *arg)
{
    int idx = 0;
    int pos = 0;
	while(1)
	{
		struct point p;
		++pos;
		p.x = p.y = p.z = pos+1;
		SetPoint(g_points[idx],p);
		idx = (idx + 1)%100;
	}
}

void *GetRoutine(void *arg)
{
    int idx = 0;
	while(1)
	{
		struct point ret = GetPoint(g_points[idx]);
		if(ret.x != ret.y || ret.x != ret.z || ret.y != ret.z)
		{
			printf("%d,%d,%d\n",ret.x,ret.y,ret.z);
			assert(0);
		}			
		idx = (idx + 1)%100;
	}
}

int main()
{
	struct point p;
	p.x = p.y = p.z = 1;
	int i = 0;
	for(; i < 1000; ++i)
	{
		g_points[i] = create_atomic_type(sizeof(p));
		SetPoint(g_points[i],p);
	}
	thread_t t1 = CREATE_THREAD_RUN(1,SetRotine,NULL);
	thread_t t2 = CREATE_THREAD_RUN(1,GetRoutine,(void*)1);	
	thread_t t3 = CREATE_THREAD_RUN(1,GetRoutine,(void*)1);	
	thread_t t4 = CREATE_THREAD_RUN(1,GetRoutine,(void*)1);	
	uint32_t tick = GetSystemMs();
	while(1)
	{
		uint32_t new_tick = GetSystemMs();
		if(new_tick - tick >= 1000)
		{
			printf("get:%d,set:%d,miss:%d\n",get_count,set_count,miss_count);
			get_count = set_count = miss_count = 0;
			tick = new_tick;
		}
		sleepms(50);
	}
}
