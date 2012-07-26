#include "sync.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct mutex
{
	pthread_mutex_t     m_mutex;
	pthread_mutexattr_t m_attr;
}mutex;

typedef struct condition
{
	pthread_cond_t cond;
}condition;

typedef struct barrior
{
	condition_t cond;
	mutex_t 	mtx;
	int32_t         wait_count;
}barrior;

mutex_t mutex_create()
{
	mutex_t m = malloc(sizeof(mutex));
	pthread_mutexattr_init(&m->m_attr);
	pthread_mutexattr_settype(&m->m_attr,PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&m->m_mutex,&m->m_attr);
	return m;
}

void mutex_destroy(mutex_t *m)
{
	pthread_mutexattr_destroy(&(*m)->m_attr);
	pthread_mutex_destroy(&(*m)->m_mutex);
	free(*m);
	*m=0;
}
inline int32_t mutex_lock(mutex_t m)
{
	return pthread_mutex_lock(&m->m_mutex);
}
inline int32_t mutex_try_lock(mutex_t m)
{
	return pthread_mutex_trylock(&m->m_mutex);
}
inline int32_t mutex_unlock(mutex_t m)
{
	return pthread_mutex_unlock(&m->m_mutex);
}



condition_t condition_create()
{
	condition_t c = malloc(sizeof(condition));
	pthread_cond_init(&c->cond,NULL);
	return c;
}

void condition_destroy(condition_t *c)
{
	pthread_cond_destroy(&(*c)->cond);
	free(*c);
	*c = 0;
}

inline int32_t condition_wait(condition_t c,mutex_t m)
{
	return pthread_cond_wait(&c->cond,&m->m_mutex);
}
#include <sys/time.h>
int32_t condition_timedwait(condition_t c,mutex_t m,int32_t ms)
{
   struct timespec ts;
   //clock_gettime(1/*CLOCK_REALTIME*/, &ts);
   //ts.tv_sec += 1;
   struct timeval now;
   gettimeofday(&now, NULL);
   ts.tv_sec = now.tv_sec + 5;
   ts.tv_nsec = now.tv_usec * 1000;   
	
	return pthread_cond_timedwait(&c->cond,&m->m_mutex,&ts);
}

inline int32_t condition_signal(condition_t c)
{
	return pthread_cond_signal(&c->cond);
}

inline int32_t condition_broadcast(condition_t c)
{
	return pthread_cond_broadcast(&c->cond);
}

barrior_t barrior_create(int waitcount)
{
	barrior_t b = malloc(sizeof(barrior));
	b->wait_count = waitcount;
	b->mtx = mutex_create();
	b->cond = condition_create();
	return b;
}

void barrior_destroy(barrior_t *b)
{
	mutex_destroy(&(*b)->mtx);
	condition_destroy(&(*b)->cond);
	free(*b);
	*b = 0;
}

void barrior_wait(barrior_t b)
{
	mutex_lock(b->mtx);
	--b->wait_count;
	if(0 == b->wait_count)
	{
		condition_broadcast(b->cond);
	}else	
	{
		while(b->wait_count > 0)
		{
			condition_wait(b->cond,b->mtx);
		}
	}
	mutex_unlock(b->mtx);
}
