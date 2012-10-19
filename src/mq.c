#include <stdlib.h>
#include "mq.h"
#include <pthread.h>
#include "sync.h"
#include "double_link.h"
#include "link_list.h"

struct per_thread_struct
{
	list_node   next;
	struct double_link_node block;
	struct link_list *local_q;
	condition_t cond;
};

struct mq
{
	uint32_t           push_size;
	pthread_key_t      t_key;
	mutex_t            mtx;
	struct double_link blocks;
	struct link_list  *share_list;
	struct link_list  *local_lists;
	
};

static struct per_thread_struct *per_thread_create()
{
	struct per_thread_struct *pts = calloc(1,sizeof(*pts));
	pts->local_q = LINK_LIST_CREATE();
	pts->cond = condition_create();
	return pts;
}

static void per_thread_destroy(struct per_thread_struct **pts)
{
	LINK_LIST_DESTROY(&(*pts)->local_q);
	condition_destroy(&(*pts)->cond);
	free(*pts);
	*pts = NULL;
}


mq_t create_mq(uint32_t push_size)
{
	mq_t m = calloc(1,sizeof(*m));
	m->mtx = mutex_create();
	pthread_key_create(&m->t_key,0);
	m->share_list = LINK_LIST_CREATE();
	m->local_lists = LINK_LIST_CREATE();
	double_link_clear(&m->blocks);
	m->push_size = push_size;
	return m; 	
}

void destroy_mq(mq_t *m)
{
	mutex_destroy(&(*m)->mtx);
	LINK_LIST_DESTROY(&(*m)->share_list);
	list_node *l = link_list_head((*m)->local_lists);
	while(l)
	{
		struct per_thread_struct *pts = (struct per_thread_struct*)l;
		l = l->next;
		per_thread_destroy(&pts);
	}	
	LINK_LIST_DESTROY(&(*m)->local_lists);
	free(*m);
	*m = NULL;		
}

static inline mq_sync_push(mq_t m,struct per_thread_struct *pts)
{
	mutex_lock(m->mtx);
	uint8_t empty = link_list_is_empty(m->share_list);
	link_list_swap(m->share_list,pts->local_q);
	if(empty)
	{
		struct double_link_node *l = double_link_pop(&m->blocks);
		if(l)
		{
			//if there is block per_thread_struct wake it up
			struct per_thread_struct *block_pts = (struct per_thread_struct *)((uint8_t*)l - sizeof(struct list_node));
			mutex_unlock(m->mtx);
			condition_signal(block_pts->cond);
			return;
		}
	}
	mutex_unlock(m->mtx);
}

static inline mq_sync_pop(mq_t m,struct per_thread_struct *pts,uint32_t timeout)
{
	mutex_lock(m->mtx);
	if(link_list_is_empty(m->share_list))
	{
		if(timeout)
		{	
			while(link_list_is_empty(m->share_list))
			{
				double_link_push(&m->blocks,&pts->block);
				if(0 != condition_timedwait(pts->cond,m->mtx,timeout))
				{
					double_link_remove(&pts->block);
					break;
				}
			}
		}
	}
	link_list_swap(pts->local_q,m->share_list);
	mutex_unlock(m->mtx);
}

void mq_push(mq_t m,struct list_node *msg)
{
	struct per_thread_struct *pts = (struct per_thread_struct*)pthread_getspecific(m->t_key);
	if(!pts)
	{
		pts = per_thread_create();
		LINK_LIST_PUSH_BACK(m->local_lists,pts);
		pthread_setspecific(m->t_key,(void*)pts);
	}
	LINK_LIST_PUSH_BACK(pts->local_q,msg);
	if(link_list_size(pts->local_q) >= m->push_size)
		mq_sync_push(m,pts);		
}

void mq_push_now(mq_t m,struct list_node *msg)
{
	struct per_thread_struct *pts = (struct per_thread_struct*)pthread_getspecific(m->t_key);
	if(!pts)
	{
		pts = per_thread_create();
		LINK_LIST_PUSH_BACK(m->local_lists,pts);
		pthread_setspecific(m->t_key,(void*)pts);
	}
	LINK_LIST_PUSH_BACK(pts->local_q,msg);
	//if(link_list_size(pts->local_q) >= m->push_size)
	mq_sync_push(m,pts);		
}

struct list_node* mq_pop(mq_t m,uint32_t timeout)
{
	struct per_thread_struct *pts = (struct per_thread_struct*)pthread_getspecific(m->t_key);
	if(!pts)
	{
		pts = per_thread_create();
		LINK_LIST_PUSH_BACK(m->local_lists,pts);
		pthread_setspecific(m->t_key,(void*)pts);
	}
	if(link_list_is_empty(pts->local_q))
		mq_sync_pop(m,pts,timeout);
	return LINK_LIST_POP(struct list_node*,pts->local_q);
}


void   mq_swap(mq_t m,struct link_list *l,uint32_t timeout)
{
	struct per_thread_struct *pts = (struct per_thread_struct*)pthread_getspecific(m->t_key);
	if(!pts)
	{
		pts = per_thread_create();
		LINK_LIST_PUSH_BACK(m->local_lists,pts);
		pthread_setspecific(m->t_key,(void*)pts);
	}
	if(link_list_is_empty(pts->local_q))
		mq_sync_pop(m,pts,timeout);
	link_list_swap(l,pts->local_q);
}


void mq_force_sync(mq_t m)
{
	struct per_thread_struct *pts = (struct per_thread_struct*)pthread_getspecific(m->t_key);
	if(!pts)
	{
		pts = per_thread_create();
		LINK_LIST_PUSH_BACK(m->local_lists,pts);
		pthread_setspecific(m->t_key,(void*)pts);
		return;
	}	
	if(link_list_is_empty(pts->local_q) == 0)
		mq_sync_push(m,pts);
}
