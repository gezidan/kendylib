#include <stdlib.h>
#include "mq.h"
#include <pthread.h>
#include "spinlock.h"


struct localQ
{
	list_node next;
	struct link_list *local_q;
};

struct mq
{
	uint32_t          push_size;
	pthread_key_t     t_key;
	spinlock_t       mtx;
	struct link_list *share_list;
	struct link_list *local_lists;
};

struct localQ *localQ_create()
{
	struct localQ *local_q = calloc(1,sizeof(*local_q));
	local_q->local_q = LINK_LIST_CREATE();
	return local_q;
}

void localQ_destroy(struct localQ **local_q)
{
	LINK_LIST_DESTROY(&(*local_q)->local_q);
	free(*local_q);
	*local_q = NULL;
}

mq_t create_mq(uint32_t push_size)
{
	mq_t m = calloc(1,sizeof(*m));
	m->mtx = spin_create();
	pthread_key_create(&m->t_key,0);
	m->share_list = LINK_LIST_CREATE();
	m->local_lists = LINK_LIST_CREATE();
	m->push_size = push_size;
	return m; 	
}

void destroy_mq(mq_t *m)
{
	spin_destroy(&(*m)->mtx);
	LINK_LIST_DESTROY(&(*m)->share_list);
	list_node *l = link_list_head((*m)->local_lists);
	while(l)
	{
		struct localQ *lq = (struct localQ *)l;
		l = l->next;
		localQ_destroy(&lq);
	}	
	LINK_LIST_DESTROY(&(*m)->local_lists);
	free(*m);
	*m = NULL;		
}

static inline mq_sync_push(mq_t m,struct localQ *local_q)
{
	spin_lock(m->mtx);
	link_list_swap(m->share_list,local_q->local_q);
	spin_unlock(m->mtx);
}

static inline mq_sync_pop(mq_t m,struct localQ *local_q)
{
	spin_lock(m->mtx);
	link_list_swap(local_q->local_q,m->share_list);
	spin_unlock(m->mtx);
}

void mq_push(mq_t m,struct list_node *msg)
{
	struct localQ *local_q = (struct localQ *)pthread_getspecific(m->t_key);
	if(!local_q)
	{
		local_q = localQ_create();
		LINK_LIST_PUSH_BACK(m->local_lists,local_q);
		pthread_setspecific(m->t_key,(void*)local_q);
	}
	LINK_LIST_PUSH_BACK(local_q->local_q,msg);
	if(link_list_size(local_q->local_q) >= m->push_size)
		mq_sync_push(m,local_q);		
}

struct list_node* mq_pop(mq_t m)
{
	struct localQ *local_q = (struct localQ *)pthread_getspecific(m->t_key);
	if(!local_q)
	{
		local_q = localQ_create();
		LINK_LIST_PUSH_BACK(m->local_lists,local_q);
		pthread_setspecific(m->t_key,(void*)local_q);
	}
	if(link_list_is_empty(local_q->local_q))
		mq_sync_pop(m,local_q);
	return LINK_LIST_POP(struct list_node*,local_q->local_q);
}

void mq_timeout(mq_t m)
{
	struct localQ *local_q = (struct localQ *)pthread_getspecific(m->t_key);
	if(!local_q)
	{
		local_q = localQ_create();
		LINK_LIST_PUSH_BACK(m->local_lists,local_q);
		pthread_setspecific(m->t_key,(void*)local_q);
		return;
	}	
	if(link_list_is_empty(local_q->local_q) == 0)
		mq_sync_push(m,local_q);
}
