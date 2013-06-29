#include "map_RBtree.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "map.h"
#include "list.h"
#include "RBtree.h"
struct RBtree;
struct RBnode
{
	struct rbnode base;
	void   *val;
#define __key base.key
#define __val val
	struct RBtree *tree;
	uint16_t key_size;
	uint16_t val_size;
	int8_t   data[1];//key & value
};

#ifndef IMPLEMEMT
#define IMPLEMEMT(SUPER_CLASS) struct SUPER_CLASS super_class
#endif

struct RBtree
{
	IMPLEMEMT(interface_map_container);
	rbtree_t _rb;
	uint16_t key_size;
	uint16_t val_size;
};

extern map_iter RBtree_insert(struct interface_map_container *,void*,void*);
extern map_iter RBtree_erase(struct interface_map_container *,map_iter);
extern void RBtree_delete(struct interface_map_container *,void*);
extern map_iter RBtree_find(struct interface_map_container *,void *);
extern map_iter RBtree_begin(struct interface_map_container *);
extern map_iter RBtree_end(struct interface_map_container *);
extern int32_t RBtree_size(struct interface_map_container *);
extern int32_t RBtree_empty(struct interface_map_container *);

RBtree_t RBtree_create(uint16_t key_size,uint16_t val_size,comp _comp)
{
	struct RBtree *rb = malloc(sizeof(*rb));
	if(rb)
	{
		rb->_rb = create_rbtree(_comp);
		rb->key_size = key_size;
		rb->val_size = val_size;
		rb->super_class.insert = RBtree_insert;
		rb->super_class.erase = RBtree_erase;
		rb->super_class.remove = RBtree_delete;
		rb->super_class.find = RBtree_find;
		rb->super_class.begin = RBtree_begin;
		rb->super_class.end = RBtree_end;
		rb->super_class.size = RBtree_size;
		rb->super_class.empty = RBtree_empty;
		rb->super_class.destroy = RBtree_destroy;
	}
	return rb;
}

void RBtree_destroy(struct interface_map_container **_rb)
{
	struct RBtree *rb = (struct RBtree*)*_rb;
	if(rb->_rb->size)
	{
		struct RBnode *cur = (struct RBnode *)rbtree_first(rb->_rb);
		struct RBnode *next = (struct RBnode *)rbnode_next((struct rbnode*)cur);
		while(cur)
		{
			free(cur);
			cur = next;
			next = (struct RBnode*)rbnode_next((struct rbnode*)cur);
		}
	}
	free(rb);
	*_rb = 0;
}

struct RBnode *create_node(RBtree_t rb,void *key,void *value)
{

	struct RBnode *n = malloc(sizeof(*n) + rb->key_size + rb->val_size - 1);
	n->__key = &n->data[0];
	n->__val = &n->data[rb->key_size];
	n->key_size = rb->key_size;
	n->val_size = rb->val_size;
	n->tree = rb;
	memcpy(n->__key,key,rb->key_size);
	memcpy(n->__val,value,rb->val_size);
	return n;
}

void rb_iter_get_key(struct base_iterator *_iter, void *key)
{
	map_iter *iter = (map_iter*)_iter;
	struct RBnode *n = (struct RBnode*)iter->node;
	memcpy(key,n->__key,n->key_size);
}

void rb_iter_get_val(struct base_iterator *_iter, void *val)
{
	map_iter *iter = (map_iter*)_iter;
	struct RBnode *n = (struct RBnode*)iter->node;
	memcpy(val,n->__val,n->val_size);
}

void rb_iter_set_val(struct base_iterator *_iter, void *val)
{
	map_iter *iter = (map_iter*)_iter;
	struct RBnode *n = (struct RBnode*)iter->node;
	memcpy(n->__val,val,n->val_size);
}

void RB_iter_init(map_iter *,struct RBnode *);

#define CREATE_MAP_IT(IT,ARG1)\
	map_iter IT;\
	RB_iter_init(&IT,ARG1)

void RB_iter_next(struct base_iterator *_iter)
{
	map_iter *iter = (map_iter*)_iter;
	struct RBnode *n = (struct RBnode*)iter->node;
	RBtree_t rb = n->tree;
	if(iter->node == rb->_rb->nil)
		return;
	struct RBnode *succ = (struct RBnode*)rbnode_next((struct rbnode*)n);
	if(!succ)
		iter->node = rb->_rb->nil;
	else
		iter->node = succ;
}

int8_t RB_iter_equal(struct base_iterator *_a,struct base_iterator *_b)
{
	map_iter *a = (map_iter*)_a;
	map_iter *b = (map_iter*)_b;
	return a->node == b->node;
}

void RB_iter_init(map_iter *iter,struct RBnode *n)
{
	iter->base.next = RB_iter_next;
	iter->base.get_key = rb_iter_get_key;
	iter->base.get_val = rb_iter_get_val;
	iter->base.set_val = rb_iter_set_val;
	iter->base.is_equal = RB_iter_equal;
	iter->node = n;
}

map_iter RBtree_begin(struct interface_map_container *_rb)
{
	RBtree_t rb = (RBtree_t)_rb;
	struct RBnode *first = (struct RBnode*)rbtree_first(rb->_rb);
	CREATE_MAP_IT(begin,NULL);
	begin.node = (first == NULL ? (struct RBnode*)rb->_rb->nil : first);
	return begin;
}

map_iter RBtree_end(struct interface_map_container *_rb)
{
	RBtree_t rb = (RBtree_t)_rb;
	CREATE_MAP_IT(end,(struct RBnode*)rb->_rb->nil);
	return end;
}

map_iter RBtree_find(struct interface_map_container *_rb,void *key)
{
	RBtree_t rb = (RBtree_t)_rb;
	CREATE_MAP_IT(it,(struct RBnode*)rbtree_find(rb->_rb,key));
	return it;
}

map_iter RBtree_insert(struct interface_map_container *_rb,void *key,void *val)
{
	RBtree_t rb = (RBtree_t)_rb;
	assert(rb);
	struct RBnode *n = create_node(rb,key,val);
	if(0 == rbtree_insert(rb->_rb,(struct rbnode*)n))
	{
		CREATE_MAP_IT(it,n);
		return it;
	}
	return RBtree_end(_rb);
}

void  RBtree_delete(struct interface_map_container *_rb,void *key)
{
	RBtree_t rb = (RBtree_t)_rb;
	struct RBnode *n = (struct RBnode*)rbtree_remove(rb->_rb,key);
	if(n)
		free(n);
}

map_iter RBtree_erase(struct interface_map_container *_rb,map_iter it)
{
	RBtree_t rb = (RBtree_t)_rb;
	struct RBnode *n = it.node;
	if(n == (struct RBnode*)rb->_rb->nil)
		return it;
	struct RBnode *succ = (struct RBnode*)rbnode_next((struct rbnode*)n);
	if(!succ)
		return RBtree_end(_rb);
	CREATE_MAP_IT(next,succ);
	return next;
}

int32_t RBtree_size(struct interface_map_container *_rb)
{
	RBtree_t rb = (RBtree_t)_rb;
	return rb->_rb->size;
}

int32_t RBtree_empty(struct interface_map_container *_rb)
{
	RBtree_t rb = (RBtree_t)_rb;
	return rb->_rb->size == 0;
}
