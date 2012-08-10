#include "data_struct.h"
#include <stdlib.h>
#include <stdio.h>

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
	printf("a db_array destroy\n");
}

db_array_t db_array_create(int32_t size)
{
	db_array_t a = calloc(1,sizeof(*a));
	a->base.type = DB_ARRAY;
	a->base.ref.mt = 0;
	a->base.ref.refcount = 1;
	a->base.ref.destroyer = db_array_destroy;
	a->size = size;
	a->data = calloc(size,sizeof(*(a->data))); 
	return a;	
}

void db_array_release(db_array_t *a)
{
	ref_decrease((struct refbase*)*a);
	*a = NULL;
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

db_list_t db_list_create()
{
	db_list_t l = calloc(1,sizeof(*l));
	return l;	
}

void      db_list_destroy(db_list_t *l)
{
	
}

int32_t   db_list_append(db_list_t l,db_array_t a)
{
	struct db_node *n = calloc(1,sizeof(*n));
	n->array = a;
	if(l->tail)
	{
		l->tail->next = n;
		l->tail = n;
	}
	else
		l->head = l->tail = n;
	return ++l->size;
}

int32_t   db_list_size(db_list_t l)
{
	return l->size;
}

int32_t   db_list_shrink(db_list_t l)
{
	struct db_node *cur = l->head;
	struct db_node *pre = NULL;
	while(cur)
	{
	    if(cur->array->data == NULL)
	    {			
			struct db_node *del = cur;
			cur = cur->next;
			if(l->head == del && pre == NULL)
				l->head = cur;
			if(l->tail == del)
			{
				if(pre)
					l->tail = pre;
				else
					l->tail = NULL;
			}
			db_array_release(&(del->array));
			free(del);
			--l->size;
		}
		else
		{
			pre = cur;
			cur = cur->next;
		}					
	}
	return l->size;
}
