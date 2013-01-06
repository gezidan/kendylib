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

volatile int get_count = 0;
volatile int set_count = 0;
volatile int miss_count = 0;
mutex_t mtx;
//#define _USE_MTX_
#ifdef _USE_MTX_
struct point g_point;
struct point GetPoint()
{
	mutex_lock(mtx);
	struct point ret;
	ret.x = g_point.x;
	ret.y = g_point.y;
	ret.z = g_point.z;
	++get_count;
	mutex_unlock(mtx);
	return ret;
}
void SetPoint(struct point p)
{
	mutex_lock(mtx);
	g_point.x = p.x;
	g_point.y = p.y;
	g_point.z = p.z;
	++set_count;
	mutex_unlock(mtx);
}

#else
struct point * volatile g_point = NULL;

struct point GetPoint()
{
	struct point ret;
	while(1)
	{
		struct point *ptr_p = g_point;
		int save_version = ptr_p->version;
		ret.x = ptr_p->x;
		ret.y = ptr_p->y;
		ret.z = ptr_p->z;
		//__sync_synchronize();
		if(ptr_p == g_point && save_version == ptr_p->version)
		{
			if(ret.x != ret.y || ret.x != ret.z || ret.y != ret.z)
			{
				printf("%d,%d,%d,%u\n",ret.x,ret.y,ret.z,save_version);
				assert(0);
			}	
			break;
		}
		++miss_count;
	}
	++get_count;
	return ret;
}

void SetPoint(struct point p)
{
	static uint32_t g_version = 0;
	struct point *old_p = g_point;
	static struct point  array[2];	
	static int32_t index  = 0;
	
	struct point *new_p = &array[index];
	index = (index + 1)%2;	
	new_p->x = p.x;
	new_p->y = p.y;
	new_p->z = p.z;
	//__sync_synchronize();
	new_p->version = ++g_version;
	//__sync_synchronize();
	g_point = new_p;
	++set_count;
}
#endif

void *SetRotine(void *arg)
{
	while(1)
	{
		struct point p;
		p.x = p.y = p.z = (rand()%10) + 1;
		SetPoint(p);
	}
}

void *GetRoutine(void *arg)
{
	while(1)
	{
		struct point p = GetPoint();
	}
}

int main()
{
	mtx = mutex_create();	
	struct point p;
	p.x = p.y = p.z = 1;
	SetPoint(p);
	thread_t t1 = CREATE_THREAD_RUN(1,SetRotine,NULL);
	thread_t t2 = CREATE_THREAD_RUN(1,GetRoutine,NULL);	
	thread_t t3 = CREATE_THREAD_RUN(1,GetRoutine,NULL);
	thread_t t4 = CREATE_THREAD_RUN(1,GetRoutine,NULL);								
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

