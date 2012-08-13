#include "data_struct.h"
#include <stdlib.h>
#include <stdio.h>
#include "SysTime.h"


void db_element_release(db_element_t *e)
{
	ref_decrease((struct refbase*)*e);
	*e = NULL;
}

db_element_t db_element_acquire(db_element_t e1,db_element_t e2)
{
	if(e1 == e2)
		return e1;	
	if(e2)
		ref_increase((struct refbase*)e2);
	if(e1)
		db_element_release(&e1);

	return e2;
}

static void db_array_destroy(void *arg)
{
	db_array_t a = (db_array_t)arg;
	if(a->data)
	{
		int32_t i = 0;
		for( ; i < a->size; ++i)
			basetype_destroy(&(a->data[i]));
		
		free(a->data);
	}
	free(a);
	printf("a db_array destroyed\n");
}

db_array_t db_array_create(int32_t size)
{
	db_array_t a = calloc(1,sizeof(*a));
	a->base.type = DB_ARRAY;
	a->base.ref.mt = 0;
	a->base.ref.destroyer = db_array_destroy;
	a->size = size;
	a->data = calloc(size,sizeof(*(a->data))); 
	a = (db_array_t)db_element_acquire(NULL,(db_element_t)a);
	return a;	
}

void db_array_release(db_array_t *a)
{
	db_element_release((db_element_t*)a);
	*a = NULL;
}

db_array_t db_array_acquire(db_array_t a1,db_array_t a2)
{
	return (db_array_t)db_element_acquire((db_element_t)a1,(db_element_t)a2); 
}


basetype_t db_array_get(db_array_t a,int32_t index)
{
	if(a->data && index < a->size)
		return a->data[index];
	return NULL;
}

void db_array_set(db_array_t a,int32_t index,basetype_t t)
{
	if(a->data && index < a->size)
	{
		if(a->data[index])
			basetype_destroy(&(a->data[index]));
		a->data[index] = t;	
	}
}

void db_array_clear(db_array_t a)
{
	int32_t i = 0;
	for( ; i < a->size; ++i)
	{
		if(a->data[i])
			basetype_destroy(&(a->data[i]));
	}
	free(a->data);
	a->data = NULL;
}

void      db_list_destroy(void *_l)
{
	db_list_t l = (db_list_t)_l;
	struct db_node *cur;
	while(cur = (struct db_node *)link_list_pop((l)->l))
	{
		db_element_release((db_element_t*)&(cur->array));
		free(cur);
	}
	
	LINK_LIST_DESTROY(&(l)->l); 
	free(l);
	printf("a db_list destroyed\n");
}

db_list_t db_list_create()
{
	db_list_t l = calloc(1,sizeof(*l));
	l->l = LINK_LIST_CREATE();
	l->base.type = DB_LIST;
	l->base.ref.destroyer = db_list_destroy;
	l = (db_list_t)db_element_acquire(NULL,(db_element_t)l);
	return l;	
}

db_list_t db_list_acquire(db_list_t l1,db_list_t l2)
{
	return (db_list_t)db_element_acquire(NULL,(db_element_t)l2);
}

void db_list_release(db_list_t *l)
{
	db_element_release((db_element_t*)l);
	*l = NULL;
}



int32_t   db_list_append(db_list_t l,db_array_t a)
{
	struct db_node *n = calloc(1,sizeof(*n));
	n->array = a;
	LINK_LIST_PUSH_BACK(l->l,n);
	//increase the reference
	ref_increase((struct refbase*)a);	
	return link_list_size(l->l);
}

int32_t   db_list_size(db_list_t l)
{
	return link_list_size(l->l);
}

int8_t   db_list_shrink(db_list_t l,uint32_t maxtime)
{
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
	return 1;
}
