#include "util/thread.h"
#include "util/SysTime.h"
#include "util/atomic.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct point
{
	volatile int version;
	volatile int x;
	volatile int y;
	volatile int z;
};

volatile int get_count = 0;
volatile int set_count = 0;

struct point *g_point = NULL;

struct point GetPoint()
{
	struct point ret;
	while(1)
	{
		struct point *ptr_p = g_point;
		int save_version = ptr_p->version;
		if(save_version & 0xABCD0000)
		{
			ret.x = ptr_p->x;
			ret.y = ptr_p->y;
			ret.z = ptr_p->z;
			int new_version = ptr_p->version;
			if(ptr_p == g_point && (new_version & 0xABCD0000) && save_version == new_version)
			{
				if(ret.x != ret.y || ret.x != ret.z || ret.y != ret.z)
				{
					printf("%d,%d,%d,%u\n",ret.x,ret.y,ret.z,new_version);
					assert(0);
				}
				if(ret.x == 0 || ret.y == 0 || ret.z == 0)
				{
					printf("%d,%d,%d,%u\n",ret.x,ret.y,ret.z,new_version);
					assert(0);
				}			
				break;
			}
	    }
	}
	++get_count;
	return ret;
}

void SetPoint(struct point p)
{
	static uint32_t g_version = 0;
	struct point *old_p = g_point;
	struct point *new_p = calloc(1,sizeof(*new_p));	
	new_p->x = p.x;
	new_p->y = p.y;
	new_p->z = p.z;
	++g_version;
	new_p->version = 0xABCD0000 + (g_version%65535);
	__sync_synchronize();
	g_point = new_p;
	if(old_p)
	{
		old_p->version = 0;
		old_p->x = old_p->y = old_p->z = 0;
		free(old_p);
	}
	++set_count;
}

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
	struct point p;
	p.x = p.y = p.z = 1;
	SetPoint(p);
	thread_t t1 = CREATE_THREAD_RUN(1,SetRotine,NULL);
	thread_t t2 = CREATE_THREAD_RUN(1,GetRoutine,NULL);	
	uint32_t tick = GetSystemMs();
	while(1)
	{
		uint32_t new_tick = GetSystemMs();
		if(new_tick - tick >= 1000)
		{
			printf("get:%d,set:%d\n",get_count,set_count);
			get_count = set_count = 0;
			tick = new_tick;
		}
		sleepms(10);
	}
	//getchar();
	return 0;
}

