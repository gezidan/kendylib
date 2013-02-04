#include "map_RBtree.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "map.h"
#include "list.h"

typedef struct RBnode
{
	struct RBnode *parent;
	struct RBnode *left;
	struct RBnode *right;
	RBtree_t tree;
	uint16_t color;
	uint16_t key_size;
	uint16_t val_size;
	int8_t   data[1];//key & value
}RBnode;

#ifndef IMPLEMEMT
#define IMPLEMEMT(SUPER_CLASS) struct SUPER_CLASS super_class
#endif

struct RBtree
{
	IMPLEMEMT(interface_map_container);
	RBnode *root;
	RBnode *nil;
	uint32_t size;
	uint16_t key_size;
	uint16_t val_size;
	comp    compare_function;
	RBnode  dummy;//nil节点
};

#define RED 1
#define BLACK 2

//节点数据操作
inline static void *get_key(RBnode *n)
{
	assert(n);
	return (void*)(n->data);
}

inline static void *get_value(RBnode *n)
{
	assert(n);
	return (void*)(&(n->data[n->key_size]));
}

inline static void copy_key(RBnode *n,void *key)
{
	memcpy(get_key(n),key,n->key_size);
}

inline static void copy_value(RBnode *n,void *value)
{
	memcpy(get_value(n),value,n->val_size);
}

inline static int32_t less(RBtree_t rb,void *left,void *right)
{
	return rb->compare_function(left,right) == -1;
}

inline static int32_t equal(RBtree_t rb,void *left,void *right)
{
	return rb->compare_function(left,right) == 0;
}

inline static int32_t more(RBtree_t rb,void *left,void *right)
{
	return rb->compare_function(left,right) == 1;
}

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
		rb->size = 0;
		rb->dummy.color = BLACK;
		rb->nil = &rb->dummy;
		rb->nil->tree = rb;
		rb->root = rb->nil;
		rb->key_size = key_size;
		rb->val_size = val_size;
		rb->compare_function = _comp;
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
	if(rb->size)
	{
		list_t stack = list_create(sizeof(RBnode*));
		LIST_PUSH_FRONT(RBnode*,stack,rb->root);
		for( ; !list_is_empty(stack); )
		{
			RBnode *cur = LIST_FRONT(RBnode*,stack);
			if(cur->right)
			{
				LIST_PUSH_FRONT(RBnode*,stack,cur->right);
				cur->right = 0;
			}
			if(cur->left)
			{
				LIST_PUSH_FRONT(RBnode*,stack,cur->left);
				cur->left = 0;
			}
			if(!cur->left && !cur->right)
			{
				LIST_POP_FRONT(RBnode*,stack);
				free(cur);
				cur = 0;
			}
		}
		list_destroy(&stack);
	}
	free(rb);
	*_rb = 0;

}

/*
*   左右旋转
*/
static RBnode *rotate_left(RBtree_t rb,RBnode *n)
{
	RBnode *parent = n->parent;
	RBnode *right  = n->right;
	if(right == rb->nil)
		return;	
	
	n->right = right->left;
	right->left->parent = n;
	
	if(n == rb->root)
		rb->root = right;
	else
	{
		if(n == parent->left)
			parent->left = right;
		else
			parent->right = right;
	}
	right->parent = parent;
	n->parent = right;
	right->left = n;	
}


static   RBnode *rotate_right(RBtree_t rb,RBnode *n)
{
	RBnode *parent = n->parent;
	RBnode *left  = n->left;
	if(left == rb->nil)
		return;	
	n->left = left->right;
	left->right->parent = n;
	
	if(n == rb->root)
		rb->root = left;
	else
	{
		if(n == parent->left)
			parent->left = left;
		else
			parent->right = left;
	}
	left->parent = parent;
	n->parent = left;
	left->right = n;
} 

inline static void color_flip(RBnode *n)
{
	if(n->left && n->right)
	{
		n->color = RED;
		n->left->color = n->right->color = BLACK;
	}
}

static void insert_fix_up(RBtree_t rb,RBnode *n)
{
	while(n->parent->color == RED)
	{
		RBnode *parent = n->parent;
		RBnode *grand_parent = parent->parent;
		if(parent == grand_parent->left)
		{
			RBnode *ancle = grand_parent->right;
			if(ancle->color == RED)
			{
				//叔叔节点是红色，执行一次颜色翻转
				color_flip(grand_parent);
				n = grand_parent;
			}
			else
			{
				if(n == parent->right)
				{
					n = parent;
					rotate_left(rb,n);
				}
				
				n->parent->color = BLACK;
				n->parent->parent->color = RED;
				rotate_right(rb,n->parent->parent);
			}
		}
		else
		{
			RBnode *ancle = grand_parent->left;
			if(ancle->color == RED)
			{
				//叔叔节点是红色，执行一次颜色翻转
				color_flip(grand_parent);
				n = grand_parent;
			}
			else
			{
				if(n == parent->left)
				{
					n = parent;
					rotate_right(rb,n);
				}
				n->parent->color = BLACK;
				n->parent->parent->color = RED;
				rotate_left(rb,n->parent->parent);
			}
		}
	}
	rb->root->color = BLACK;
}

static RBnode *find(RBtree_t rb,void *key)
{
	if(rb->root == rb->nil)
		return rb->nil;
	RBnode *cur = rb->root;
	RBnode *pre;
	while(cur != rb->nil)
	{
		pre = cur;
		if(equal(rb,key,get_key(cur)))
			return cur;
		if(less(rb,key,get_key(cur)))
			cur = cur->left;
		else
			cur = cur->right;
	}
	return pre;
}

RBnode *create_node(RBtree_t rb,void *key,void *value)
{
	
	RBnode *n = malloc(sizeof(*n) + rb->key_size + rb->val_size - 1);
	n->key_size = rb->key_size;
	n->val_size = rb->val_size;	
	copy_key(n,key);
	copy_value(n,value);
	return n;
}

inline static RBnode *minimum(RBtree_t rb,RBnode *n)
{
	while(n->left != rb->nil)
		n = n->left;
	return n;
}

inline static RBnode *maxmum(RBtree_t rb,RBnode *n)
{
	while(n->right != rb->nil)
		n = n->right;
	return n;
}

//获得n的后继节点,为根节点的情况下返回nil
static RBnode *successor(RBtree_t rb,RBnode *n)
{
	assert(rb);
	if(n->right != rb->nil)
		return minimum(rb,n->right);
	RBnode *y = n->parent;
	while(y != rb->nil && n == y->right)
	{
		n = y;
		y = y->parent;
	}
	return y;	
}

//获得n的前驱节点,为根节点的情况下返回nil
static RBnode *predecessor(RBtree_t rb,RBnode *n)
{
	assert(rb);
	if(n->left != rb->nil)
		return maxmum(rb,n->left);
	RBnode *y = n->parent;
	while(y != rb->nil && n == y->left)
	{
		n = y;
		y = y->parent;
	}
	return y;	
	
}

static RBnode *get_delete_node(RBtree_t rb,RBnode *n)
{
	if(n->left == rb->nil && n->right == rb->nil)
		return n;
	else if(n->right != rb->nil)
		return minimum(rb,n->right);
	else
		return maxmum(rb,n->left);
}

static void delete_fix_up(RBtree_t rb,RBnode *n)
{
	while(n != rb->root && n->color != RED)
	{
		RBnode *p = n->parent;
		if(n == p->left)
		{
			RBnode *w = p->right;
			if(w->color == RED)//兄弟为红
			{
				w->color = BLACK;
				p->color = RED;
				rotate_left(rb,p);
				w = p->right;
			}
			//兄弟为黑
			if(w->left->color == BLACK && w->right->color == BLACK)
			{
				//兄弟的两儿子为黑
				w->color = RED;
				n = p;
			}
			else
			{
				if(w->right->color == BLACK)
				{
					//兄弟的右孩子为黑
					w->left->color == BLACK;
					w->color = RED;
					rotate_right(rb,w);
					w = p->right;
				}
				//兄弟的右孩子为红
				w->color = p->color;
				p->color = BLACK;
				w->right->color = BLACK;
				rotate_left(rb,p);
				n = rb->root;
			}
		}
		else
		{
			RBnode *w = p->left;
			if(w->color == RED)
			{
				w->color = BLACK;
				p->color = RED;
				rotate_right(rb,p);
				w = p->left;
			}
			if(w->left->color == BLACK && w->right->color == BLACK)
			{
				w->color = RED;
				n = p;
			}
			else 
			{
				if(w->left->color == BLACK)
				{
					w->right->color == BLACK;
					w->color = RED;
					rotate_left(rb,w);
					w = p->left;
				}
				w->color = p->color;
				p->color = BLACK;
				w->left->color = BLACK;
				rotate_right(rb,p);
				n = rb->root;
			}				
		}
	}
	n->color = BLACK;
}

void rb_iter_get_key(struct base_iterator *_iter, void *key)
{
	map_iter *iter = (map_iter*)_iter;
	RBnode *n = (RBnode*)iter->node;
	memcpy(key,get_key(n),n->key_size);
}

void rb_iter_get_val(struct base_iterator *_iter, void *val)
{
	map_iter *iter = (map_iter*)_iter;
	RBnode *n = (RBnode*)iter->node;
	memcpy(val,get_value(n),n->val_size);
}

void rb_iter_set_val(struct base_iterator *_iter, void *val)
{
	map_iter *iter = (map_iter*)_iter;
	RBnode *n = (RBnode*)iter->node;
	copy_value(n,val);
}

void RB_iter_init(map_iter *,RBnode *);

#define CREATE_MAP_IT(IT,ARG1)\
	map_iter IT;\
	RB_iter_init(&IT,ARG1)

void RB_iter_next(struct base_iterator *_iter)
{
	map_iter *iter = (map_iter*)_iter;
	RBnode *n = (RBnode*)iter->node; 
	RBtree_t rb = n->tree;
	if(iter->node == rb->nil)
		return;
	RBnode *succ = successor(rb,n);	
	if(!succ)
		iter->node = rb->nil;
	else
		iter->node = succ;
}

int8_t RB_iter_equal(struct base_iterator *_a,struct base_iterator *_b)
{
	map_iter *a = (map_iter*)_a;
	map_iter *b = (map_iter*)_b;
	return a->node == b->node;
}

void RB_iter_init(map_iter *iter,RBnode *n)
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
	RBnode *min = minimum(rb,rb->root);
	CREATE_MAP_IT(begin,NULL);
	begin.node = (min == 0 ? rb->nil : min);
	return begin;
}

map_iter RBtree_end(struct interface_map_container *_rb)
{
	RBtree_t rb = (RBtree_t)_rb;
	CREATE_MAP_IT(end,rb->nil);	
	return end;
}

map_iter RBtree_find(struct interface_map_container *_rb,void *key)
{
	RBtree_t rb = (RBtree_t)_rb;
	RBnode *n = find(rb,key);
	if(n == rb->nil || equal(rb,key,get_key(n)) == 0)
		return RBtree_end(_rb);
	CREATE_MAP_IT(it,n);	
	return it;
}

map_iter RBtree_insert(struct interface_map_container *_rb,void *key,void *val)
{
	RBtree_t rb = (RBtree_t)_rb;
	assert(rb);	
	RBnode *x = find(rb,key);
	if(x != rb->nil && equal(rb,key,get_key(x)))
		return RBtree_end(_rb);//不允许插入重复节点
	RBnode *n = create_node(rb,key,val);
	n->color = RED;
	n->left = n->right = rb->nil;
	n->tree = rb;
	
	if(x == rb->nil)
	{
		n->parent = rb->nil;
		rb->root = n;
	}
	else
	{
		n->parent = x;
		if(less(rb,key,get_key(x)))
			x->left = n;
		else 
			x->right = n;
	}
	++rb->size;
	insert_fix_up(rb,n);
	CREATE_MAP_IT(it,n);
	return it;
}

static void rb_delete(RBtree_t rb,void *key,RBnode **succ)
{
	assert(rb);
	RBnode *n = find(rb,key);
	if(equal(rb,key,get_key(n)))
	{		
		RBnode *x = get_delete_node(rb,n);//获得实际被删除的节点
		
		if(succ)
		{
			if(x == n || less(rb,get_key(x),get_key(n)))
				*succ = successor(rb,x);
			else
			{	
				//被删除节点是自己的后继,后继的数据回被拷贝到当前节点
				//所以删除以后,当前节点就是后继节点.
				*succ = n;
			}
		}
		RBnode *parent = x->parent;
		RBnode **link = (x == parent->left)? &(parent->left):&(parent->right);
		RBnode *z = rb->nil;	
		if(x->left != rb->nil || x->right != rb->nil)
		{
			if(x->left != rb->nil)
				*link = x->left;
			else
				*link = x->right;
			z = *link;
		}
		else
			*link = rb->nil;
		
		z->parent = parent;
		if(n != x)
		{
			//拷贝数据
			copy_key(n,get_key(x));
			copy_value(n,get_value(x));
		}
		if(x != rb->root && x->color == BLACK)
			delete_fix_up(rb,z);
		if(x == rb->root)
			rb->root = rb->nil;
		free(x);		
	    --rb->size;
	}	
}

void  RBtree_delete(struct interface_map_container *_rb,void *key)
{
	RBtree_t rb = (RBtree_t)_rb;
	rb_delete(rb,key,0);
}

/*
* 删除当前节点返回后继节点的迭代器
*/
map_iter RBtree_erase(struct interface_map_container *_rb,map_iter it) 
{
	RBtree_t rb = (RBtree_t)_rb;
	RBnode *succ = 0;
	rb_delete(rb,get_key(it.node),&succ);
	if(succ == 0)
		return RBtree_end(_rb);
	CREATE_MAP_IT(next,succ);
	return next;	
}

RBtree_size(struct interface_map_container *_rb)
{
	RBtree_t rb = (RBtree_t)_rb;
	return rb->size;
}

RBtree_empty(struct interface_map_container *_rb)
{
	RBtree_t rb = (RBtree_t)_rb;
	return rb->size == 0;
}


static int32_t check(RBtree_t rb,RBnode *n,int32_t level,int32_t black_level,int32_t *max_black_level,int32_t *max_level)
{
	if(n == rb->nil)
		return 1;
	if(n->color == BLACK)
		++black_level;
	else
	{
		if(n->parent->color == RED)
		{
			printf("父节点颜色为RED\n");
			return 0;
		}
	}
	++level;
	if(n->left == rb->nil && n->right == rb->nil)
	{
		//到达叶节点
		if(level > *max_level)
			*max_level = level;		
		if(*max_black_level == 0)
			*max_black_level = black_level;
		else
			if(*max_black_level != black_level)
			{
				printf("黑色节点数目不一致\n");
				return 0;
			}
		return 1;
	}
	else
	{
		if(0 == check(rb,n->left,level,black_level,max_black_level,max_level))
			return 0;
		if(0 == check(rb,n->right,level,black_level,max_black_level,max_level))
			return 0;
	}
}

void RBtree_check_vaild(RBtree_t rb)
{
	assert(rb);
	if(rb->root != rb->nil)
	{
		int32_t max_black_level = 0;
		int32_t max_level = 0;
		if(check(rb,rb->root,0,0,&max_black_level,&max_level))
			printf("max_black_level:%d,max_level:%d\n",max_black_level,max_level);
	}
}
