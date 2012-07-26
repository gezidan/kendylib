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
    if(count == 0)
	{
		while(1)
		{
			if(l->owner == 0)
			{
				if(COMPARE_AND_SWAP(&(l->owner),0,tid) == 0)
					break;
			}
			__sync_synchronize();	
		};
		++l->lock_count;
		l->lock_by_mtx = 0;
		return 0;
	}
	else
	{
		while((count--) > 0)
		{
			if(l->owner == 0)
			{
				if(COMPARE_AND_SWAP(&(l->owner),0,tid) == 0)
				{
					++l->lock_count;
					l->lock_by_mtx = 0;
					return 0;
				}
			}
			__sync_synchronize();
		}
		int32_t ret = mutex_lock(l->mtx);
		if(ret == 0)
			l->lock_by_mtx = 1;
		return ret;
	}
}

int32_t spin_unlock(spinlock_t l)
{
	pthread_t tid = pthread_self();
	if(tid == l->owner)
	{
		--l->lock_count;
		if(l->lock_count == 0)
		{
			if(l->lock_by_mtx)
			{
				l->lock_by_mtx = 0;
				mutex_unlock(l->mtx);
			}
			else
				COMPARE_AND_SWAP(&(l->owner),tid,0);
		}
		return 0;
	}
	return -1;
}
