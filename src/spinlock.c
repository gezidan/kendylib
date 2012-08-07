#include "spinlock.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "sync.h"
struct spinlock
{
	volatile   pthread_t  owner;
	int32_t    lock_count;
	mutex_t    mtx;
	int8_t     lock_by_mtx;
};


spinlock_t spin_create()
{
	spinlock_t sp = malloc(sizeof(*sp));
	sp->lock_count = 0;
	sp->owner = 0;
	sp->mtx = mutex_create();
	sp->lock_by_mtx = 0;
	return sp;
}

void spin_destroy(spinlock_t *sp)
{
	mutex_destroy(&(*sp)->mtx);
	free(*sp);
	*sp = 0;
}

int32_t spin_lock(spinlock_t l,int32_t count)
{
	pthread_t tid = pthread_self();
	if(tid == l->owner)
	{
		++l->lock_count;
		return 0;
	}
	int32_t c = 0;
	int32_t max = 0;
    if(count == 0)
	{
		
		while(1)
		{
			if(l->owner == 0)
			{
				if(COMPARE_AND_SWAP(&(l->owner),0,tid) == 0)
					break;
			}
			for(c = 0; c < (max = rand()%128); ++c)
				__asm__("pause");		
		};
		__sync_synchronize();	
		++l->lock_count;
		l->lock_by_mtx = 0;
		return 0;
	}
	else
	{
		int32_t _l = 0;
		while((count--) > 0)
		{
			if(l->owner == 0)
			{
				if(COMPARE_AND_SWAP(&(l->owner),0,tid) == 0)
				{
					_l = 1;
					break;
				}
			}
			for(c = 0; c < (max = rand()%128); ++c)
				__asm__("pause");
		}
		__sync_synchronize();
		if(_l == 0)
		{
			mutex_lock(l->mtx);
			l->lock_by_mtx = 1;
		}
		++l->lock_count;
		return 0;
	}
}

int32_t spin_unlock(spinlock_t l)
{
	pthread_t tid = pthread_self();
	if(tid == l->owner)
	{
		--l->lock_count;
		__sync_synchronize();
		if(l->lock_count == 0)
		{
			if(l->lock_by_mtx)
			{
				l->lock_by_mtx = 0;
				mutex_unlock(l->mtx);
			}
			else
				l->owner = 0;
		}
		return 0;
	}
	return -1;
}
