#include "data_struct.h"
#include <stdlib.h>
#include <stdio.h>
#include "SysTime.h"
#include "common_define.h"

static void db_array_destroy(void *arg)
{
	basetype_t a = (basetype_t)arg;
	if(a->array_data)
	{
		int32_t i = 0;
		for( ; i < ((db_array_t)a)->size; ++i)
			basetype_release(&(a->array_data[i]));
		
		free(a->array_data);
	}
	free(a);
	printf("a db_array destroyed\n");
}

static void db_list_destroy(void *_l)
{
	basetype_t l = (basetype_t)_l;
	struct db_node *cur;
	while(cur = (struct db_node *)link_list_pop((l)->l))
	{
		basetype_release(&(cur->element));	
		free(cur);
	}
	
	LINK_LIST_DESTROY(&(l)->l); 
	free(l);
	printf("a db_list destroyed\n");
}

basetype_t db_array_create(int32_t size)
{
	db_array_t a = calloc(1,sizeof(*a));
	a->base.type = DB_ARRAY;
	a->base.ref.mt = SINGLE_THREAD;
	a->base.ref.refcount = 1;
	a->base.ref.destroyer = db_array_destroy;
	a->size = size;
	((basetype_t)a)->array_data = calloc(size,sizeof(basetype_t)); 
	return (basetype_t)a;	
}


basetype_t db_array_get(db_array_t a,int32_t index)
{
	if(((basetype_t)a)->array_data && index < a->size)
	{
		basetype_acquire(NULL,((basetype_t)a)->array_data[index]);
		return ((basetype_t)a)->array_data[index];
	}
	return NULL;
}

void db_array_set(db_array_t a,int32_t index,basetype_t t)
{
	if(((basetype_t)a)->array_data && index < a->size)
		((basetype_t)a)->array_data[index] = basetype_acquire(((basetype_t)a)->array_data[index],t);	
}

void db_array_clear(db_array_t a)
{
	int32_t i = 0;
	for( ; i < a->size; ++i)
	{
		if(((basetype_t)a)->array_data[i])
			basetype_release(&(((basetype_t)a)->array_data[i]));
	}
	free(((basetype_t)a)->array_data);
	((basetype_t)a)->array_data = NULL;
}

basetype_t db_list_create()
{
	db_list_t l = calloc(1,sizeof(*l));
	((basetype_t)l)->l = LINK_LIST_CREATE();
	l->base.type = DB_LIST;
	l->base.ref.mt = SINGLE_THREAD;
	l->base.ref.refcount = 1;	
	l->base.ref.destroyer = db_list_destroy;
	return (basetype_t)l;	
}

int32_t   db_list_append(db_list_t l,basetype_t a)
{
	struct db_node *n = calloc(1,sizeof(*n));
	n->element = basetype_acquire(NULL,a);
	LINK_LIST_PUSH_BACK(((basetype_t)l)->l,n);	
	return link_list_size(((basetype_t)l)->l);
}

basetype_t db_list_pop(db_list_t l)
{
	struct db_node *n = LINK_LIST_POP(struct db_node *,((basetype_t)l)->l);
	if(n)
	{
		basetype_t b = n->element;
		return b;
	}
	return NULL;
}

int32_t   db_list_size(db_list_t l)
{
	return link_list_size(((basetype_t)l)->l);
}

int8_t   db_list_shrink(db_list_t l,uint32_t maxtime)
{
	/*
	if(maxtime == 0)
		return 1;
	uint32_t tick =GetCurrentMs();
	uint32_t end_tick = tick + maxtime;	
	int32_t s = link_list_size(l->l);
	int32_t i = 0;
	for(; i < s; ++i)
	{
		struct db_node *cur = (struct db_node *)link_list_pop(l->l);
		if(cur->array->data == NULL)
		{
			db_array_release(&(cur->array));
			free(cur);
		}else
		{
			LINK_LIST_PUSH_BACK(l->l,cur);
		}
		tick = GetCurrentMs();
		if(tick >= end_tick)
			break;
	}
	*/
	return 1;
}