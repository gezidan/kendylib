#define _GNU_SOURCE
#include <sched.h>
#include "util/thread.h"
#include "util/SysTime.h"
#include "util/atomic.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "util/sync.h"

struct point
{
	volatile int version;
	volatile int x;
	volatile int y;
	volatile int z;
};
#define _USE_MTX_

struct point_container
{
	uint32_t g_version;
	struct point  array[2];	
	int32_t index;
	mutex_t mtx;
#ifdef _USE_MTX_
	struct point p;
#else
	volatile struct point *ptr;
#endif	
};

struct point_container g_points[1000];

volatile int get_count = 0;
volatile int set_count = 0;
volatile int miss_count = 0;

#ifdef _USE_MTX_
struct point g_point;
inline struct point GetPoint(struct point_container *pc)
{
	mutex_lock(pc->mtx);
	struct point ret;
	ret.x = pc->p.x;
	ret.y = pc->p.y;
	ret.z = pc->p.z;
	mutex_unlock(pc->mtx);
	ATOMIC_INCREASE(&get_count);
	return ret;
}
inline void SetPoint(struct point_container *pc,struct point p)
{
	mutex_lock(pc->mtx);
	pc->p.x = p.x;
	pc->p.y = p.y;
	pc->p.z = p.z;
	mutex_unlock(pc->mtx);
	ATOMIC_INCREASE(&set_count);
}

#else
struct point GetPoint(struct point_container *pc)
{
	struct point ret;
	while(1)
	{
		volatile struct point *ptr_p = pc->ptr;
		int save_version = ptr_p->version;
		if(ptr_p == pc->ptr && save_version == ptr_p->version)
		{
			ret.x = ptr_p->x;
			ret.y = ptr_p->y;
			ret.z = ptr_p->z;
			__asm__ volatile("" : : : "memory");
			if(ptr_p == pc->ptr && save_version == ptr_p->version)
			{
				if(ret.x != ret.y || ret.x != ret.z || ret.y != ret.z)
				{
					printf("%d,%d,%d,%u\n",ret.x,ret.y,ret.z,save_version);
					assert(0);
				}	
				break;
			}
			ATOMIC_INCREASE(&miss_count);
		}
		else
			ATOMIC_INCREASE(&miss_count);
	}	
	ATOMIC_INCREASE(&get_count);
	return ret;
}

void SetPoint(struct point_container *pc,struct point p)
{
	struct point *new_p = &pc->array[pc->index];
	pc->index = (pc->index + 1)%2;
	new_p->x = p.x;
	new_p->y = p.y;
	new_p->z = p.z;
	__asm__ volatile("" : : : "memory");
	new_p->version = ++pc->g_version;
	__asm__ volatile("" : : : "memory");
	pc->ptr = new_p;	
	ATOMIC_INCREASE(&set_count);
}
#endif

void *SetRotine(void *arg)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
		fprintf(stderr, "set thread affinity failed\n");
    }
    int idx = 0;
    int pos = 0;
	while(1)
	{
		struct point p;
		++pos;
		p.x = p.y = p.z = pos+1;
		SetPoint(&g_points[idx],p);
		idx = (idx + 1)%1;
	}
}

void *GetRoutine(void *arg)
{
	int n = (int)arg;	
	cpu_set_t mask;
	CPU_ZERO(&mask);
    CPU_SET(n, &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
		fprintf(stderr, "set thread affinity failed\n");
    }
    int idx = 0;
	while(1)
	{
		struct point p = GetPoint(&g_points[idx]);
		idx = (idx + 1)%1;
	}
}

int main()
{	
	struct point p;
	p.x = p.y = p.z = 1;
	int i = 0;
	for(; i < 1000; ++i)
	{
		g_points[i].mtx = mutex_create();
		SetPoint(&g_points[i],p);
	}
	thread_t t1 = CREATE_THREAD_RUN(1,SetRotine,NULL);
	thread_t t2 = CREATE_THREAD_RUN(1,GetRoutine,(void*)1);
	thread_t t3 = CREATE_THREAD_RUN(1,GetRoutine,(void*)2);	
	thread_t t4 = CREATE_THREAD_RUN(1,GetRoutine,(void*)3);							
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
	//getchar();
	return 0;
}

