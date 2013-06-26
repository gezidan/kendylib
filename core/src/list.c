#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "list.h"

struct node
{
	struct node *next;
	struct node *pre;
	uint32_t val_size;
	union{
		int8_t   value[1];
		uint32_t pad;
	};
};

struct list
{
	uint32_t size;
	struct node head;
	struct node end;
};

#define CREATE_LIST_IT(IT)\
	struct list_iter IT;\
	list_iter_init(&IT)

static inline void list_iter_init(struct list_iter *it);

static inline void list_iter_next(struct base_iterator *_it)
{
	struct list_iter *it = (struct list_iter *)_it;
	if(it->next == NULL)
		return;
	struct node *n = (*it->next);
	if(it->next == &it->n->next)
		it->next = &(n->next);
	else if(it->next == &it->n->pre)
		it->next = &(n->pre);
	else
		assert(0);
	it->n = n;	
}

static inline int8_t list_iter_is_equal(struct base_iterator *_a,struct base_iterator *_b)
{
	struct list_iter *a = (struct list_iter *)_a;
	struct list_iter *b = (struct list_iter *)_b;
	return a->n == b->n;
}

static inline void list_iter_get_val(struct base_iterator *_it,void *v)
{
	struct list_iter *it = (struct list_iter *)_it;
	struct node *n = it->n;
	memcpy(v,n->value,n->val_size);
}

static inline void  list_iter_set_val(struct base_iterator *_it,void *v)
{
	struct list_iter *it = (struct list_iter *)_it;
	struct node *n = it->n;
	memcpy(n->value,v,n->val_size);
}

static inline void list_iter_init(struct list_iter *it)
{
	it->base.next = list_iter_next;
	it->base.get_key = NULL;
	it->base.get_val = list_iter_get_val;
	it->base.set_val = list_iter_set_val;
	it->base.is_equal = list_iter_is_equal;
}

struct list* list_create(uint32_t val_size)
{
	struct list *_list = calloc(1,sizeof(*_list));
	if(_list)
	{
		_list->size = 0;
		_list->head.val_size = _list->end.val_size = val_size;
		_list->head.next = &_list->end;
		_list->end.pre = &_list->head;
		_list->head.pre = _list->end.next = 0;
	}
	return _list;
}

void   list_destroy(struct list **_list)
{
	assert(_list);
	assert(*_list);
	if((*_list)->size > 0)
	{
		struct node *cur = (*_list)->head.next;
		while(cur != &(*_list)->end)
		{
			struct node *next = cur->next;
			free(cur);
			cur = next;
		}
	}
	free(*_list);
	*_list = 0;
}

inline struct list_iter list_begin(struct list *_list)
{
	assert(_list);
	CREATE_LIST_IT(it);
	it.n = _list->head.next;
	it.next = &(it.n->next);
	return it;
}

inline struct list_iter list_end(struct list *_list)
{
	assert(_list);
	CREATE_LIST_IT(it);
	it.n = &_list->end;
	it.next = 0;
	return it;
}

inline struct list_iter list_rbegin(struct list *_list)
{
	assert(_list);
	CREATE_LIST_IT(it);
	it.n = _list->end.pre;
	it.next = &(it.n->pre);
	return it;
}

inline struct list_iter list_rend(struct list *_list)
{
	assert(_list);
	CREATE_LIST_IT(it);
	it.n = &_list->head;
	it.next = 0;
	return it;
}

inline uint32_t list_size(struct list *_list)
{
	assert(_list);
	return _list->size;
}

void   list_insert_after(struct list *l,struct list_iter it,void *val)
{
	assert(l);
	struct node *new_node = (struct node*)calloc(1,sizeof(*new_node) + l->head.val_size - sizeof(new_node->pad));
	if(new_node)
	{
		new_node->val_size = l->head.val_size;
		memcpy(new_node->value,val,l->head.val_size);
		struct node *n = it.n;
		struct node *N = n->next;
		n->next = N->pre = new_node;
		new_node->next = N;
		new_node->pre = n;
		++l->size;
	}
}

void   list_insert_before(struct list *l, struct list_iter it,void *val)
{
	assert(l);
	struct node *new_node = (struct node*)calloc(1,sizeof(*new_node) + l->head.val_size - sizeof(new_node->pad));
	if(new_node)
	{
		new_node->val_size = l->head.val_size;
		memcpy(new_node->value,val,l->head.val_size);	
		struct node *n = it.n;
		struct node *P = n->pre;
		n->pre = P->next = new_node;
		new_node->next = n;
		new_node->pre = P;
		++l->size;
	}	
}

void list_push_back(struct list *_list,void *val)
{
	assert(_list);
	struct list_iter end = list_end(_list);
	list_insert_before(_list,end,val);
}

void list_push_front(struct list *_list,void *val)
{
	assert(_list);
	struct list_iter begin = list_begin(_list);
	list_insert_before(_list,begin,val);	
}

void  list_pop_back(struct list *_list,void *out)
{
	assert(_list);
	if(_list->size > 0)
	{
		struct node *_node = _list->end.pre;
		if(out)
			memcpy(out,_node->value,_node->val_size);
		struct node *pre = _node->pre;
		struct node *next = _node->next;
		pre->next = next;
		next->pre = pre;
		free(_node);
		//free(_node);
		--_list->size;
	}
}


void   list_back(struct list *_list ,void *out)
{
	assert(_list);
	if(_list->size > 0)
	{
		struct node *_node = _list->end.pre;
		memcpy(out,_node->value,_node->val_size);
	}	
}

void  list_pop_front(struct list *_list,void *out)
{
	assert(_list);
	if(_list->size > 0)
	{
		struct node *_node = _list->head.next;
		if(out)
			memcpy(out,_node->value,_node->val_size);
		struct node *pre = _node->pre;
		struct node *next = _node->next;
		pre->next = next;
		next->pre = pre;
		free(_node);
		--_list->size;
	}
}

void   list_front(struct list *_list,void *out)
{
	assert(_list);
	if(_list->size > 0)
	{
		struct node *_node = _list->head.next;
		memcpy(out,_node->value,_node->val_size);
	}
}

inline int32_t list_is_empty(struct list *_list)
{
	assert(_list);
	return _list->size == 0;
}

struct list_iter list_find(struct list *l,void *v)
{
	assert(l);
	CREATE_LIST_IT(it);
	it.n = NULL;
	struct node *cur = l->head.next;
	while(cur != &l->end)
	{
	
		if(memcmp(cur->value,v,l->head.val_size) == 0)
			//ÕÒµ½Ä¿±ê
			break;
		cur = cur->next;
	}

	if(cur != &l->end)
	{
		it.n = cur;
		it.next = &cur->next;
	}
	else
	{
		it.n = &l->end;
		it.next = 0;
	}
	return it;
}

int32_t list_remove(struct list *l,void *v)
{
	assert(l);
	struct list_iter it = list_find(l,v);
	if(it.n == 0)
		return 0;
	list_erase(l,it);
	return 1;
}

struct list_iter list_erase(struct list *l,struct list_iter it)
{
	assert(l);
	struct node *n = it.n;
	IT_NEXT(it);
	struct node *P = n->pre;
	struct node *N = n->next;
	P->next = N;
	N->pre = P;
	free(n);	
	--l->size;
	return it;	
}

