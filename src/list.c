#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "list.h"
#include "fix_obj_pool.h"
#include "allocator.h"
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
	struct allocator *obj_pool;//产生node使用的内存池
};

struct allocator *list_create_obj_pool(uint32_t val_size,int32_t default_size,int32_t align4)
{
	struct node dummmy;
	uint32_t node_size = sizeof(dummmy) + val_size - sizeof(dummmy.pad); 
	struct allocator *pool = create_pool(node_size,default_size,align4);
	return pool;
}

struct list* list_create(uint32_t val_size,struct allocator *obj_pool)
{
	struct list *_list = ALLOC(0,sizeof(*_list));
	if(_list)
	{
		_list->size = 0;
		_list->head.val_size = _list->end.val_size = val_size;
		_list->head.next = &_list->end;
		_list->end.pre = &_list->head;
		_list->head.pre = _list->end.next = 0;
		_list->obj_pool = obj_pool;
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
			if((*_list)->obj_pool)
				FREE((*_list)->obj_pool,cur);//pool_dealloc((*_list)->obj_pool,cur);
			else
				FREE(0,cur);
			cur = next;
		}
	}
	FREE(0,*_list);
	*_list = 0;
}

inline struct list_iter list_begin(struct list *_list)
{
	assert(_list);
	struct list_iter it;
	it.n = _list->head.next;
	it.next = &(it.n->next);
	return it;
}

inline struct list_iter list_end(struct list *_list)
{
	assert(_list);
	struct list_iter it;
	it.n = &_list->end;
	it.next = 0;
	return it;
}

inline struct list_iter list_rbegin(struct list *_list)
{
	assert(_list);
	struct list_iter it;
	it.n = _list->end.pre;
	it.next = &(it.n->pre);
	return it;
}

inline struct list_iter list_rend(struct list *_list)
{
	assert(_list);
	struct list_iter it;
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
	struct node *new_node;
	if(l->obj_pool)
		new_node = ALLOC(l->obj_pool,0);//pool_alloc(l->obj_pool,0);
	else
		new_node = ALLOC(0,sizeof(*new_node) + l->head.val_size - sizeof(new_node->pad));
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
	struct node *new_node;
	if(l->obj_pool)
		new_node = ALLOC(l->obj_pool,0);//pool_alloc(l->obj_pool,0);
	else
		new_node = ALLOC(0,sizeof(*new_node) + l->head.val_size - sizeof(new_node->pad));
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
		if(_list->obj_pool)
			FREE(_list->obj_pool,_node);//pool_dealloc(_list->obj_pool,_node);
		else
			FREE(0,_node);
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
		if(_list->obj_pool)
			FREE(_list->obj_pool,_node);//pool_dealloc(_list->obj_pool,_node);
		else
			FREE(0,_node);
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
	struct list_iter it;
	it.n = 0;
	struct node *cur = l->head.next;
	while(cur != &l->end)
	{
	
		if(memcmp(cur->value,v,l->head.val_size) == 0)
			//找到目标
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
	struct list_iter it_next = list_iter_next(it);
	struct node *n = it.n;
	struct node *P = n->pre;
	struct node *N = n->next;
	P->next = N;
	N->pre = P;
	if(l->obj_pool)
		FREE(l->obj_pool,n);//pool_dealloc(l->obj_pool,n);
	else
		FREE(0,n);	
	--l->size;
	return it_next;	
}

inline struct list_iter list_iter_next(struct list_iter it)
{
	if(it.next == 0)
		return it;
	struct list_iter it_next;
	it_next.n = (*it.next);
	if(it.next == &it.n->next)
		it_next.next = &(it_next.n->next);
	else if(it.next == &it.n->pre)
		it_next.next = &(it_next.n->pre);
	else
	{
		assert(0);
	}
	return it_next;
}

inline int32_t list_iter_is_equal(struct list_iter a,struct list_iter b)
{
	return a.n == b.n;
}

void list_iter_get_val(struct list_iter iter,void *v)
{
	struct node *n = iter.n;
	memcpy(v,n->value,n->val_size);
}

void  list_iter_set_val(struct list_iter iter,void *v)
{
	struct node *n = iter.n;
	memcpy(n->value,v,n->val_size);
}
