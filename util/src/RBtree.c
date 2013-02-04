#include "rbtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define RED 1
#define BLACK 2

rbtree_t create_rbtree(cmp_function compare_function)
{
	rbtree_t rb = malloc(sizeof(*rb));
	if(rb)
	{
		rb->size = 0;
		rb->dummy.color = BLACK;
		rb->nil = &rb->dummy;
		rb->nil->tree = rb;
		rb->root = rb->nil;
		rb->compare_function = compare_function;
	}
	return rb;
}

void destroy_rbtree(rbtree_t *rb)
{
	free(*rb);
	*rb = NULL;
}

inline static int32_t less(rbtree_t rb,void *left,void *right)
{
	return rb->compare_function(left,right) == -1;
}

inline static int32_t equal(rbtree_t rb,void *left,void *right)
{
	return rb->compare_function(left,right) == 0;
}

inline static int32_t more(rbtree_t rb,void *left,void *right)
{
	return rb->compare_function(left,right) == 1;
}


static rbnode *rotate_left(rbtree_t rb,rbnode *n)
{
	rbnode *parent = n->parent;
	rbnode *right  = n->right;
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


static   rbnode *rotate_right(rbtree_t rb,rbnode *n)
{
	rbnode *parent = n->parent;
	rbnode *left  = n->left;
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

inline static void color_flip(rbnode *n)
{
	if(n->left && n->right)
	{
		n->color = RED;
		n->left->color = n->right->color = BLACK;
	}
}

static void insert_fix_up(rbtree_t rb,rbnode *n)
{
	while(n->parent->color == RED)
	{
		rbnode *parent = n->parent;
		rbnode *grand_parent = parent->parent;
		if(parent == grand_parent->left)
		{
			rbnode *ancle = grand_parent->right;
			if(ancle->color == RED)
			{
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
			rbnode *ancle = grand_parent->left;
			if(ancle->color == RED)
			{
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

rbnode*  rbtree_find(rbtree_t rb,void *key)
{
	if(rb->root == rb->nil)
		return NULL;
	rbnode *cur = rb->root;
	rbnode *pre = NULL;
	while(cur != rb->nil)
	{
		pre = cur;
		if(equal(rb,key,cur->key))
			return cur;
		if(less(rb,key,cur->key))
			cur = cur->left;
		else
			cur = cur->right;
	}
	return pre == rb->nil? NULL:pre;
}

int8_t rbtree_insert(rbtree_t rb,rbnode *n)
{
	assert(rb);	
	rbnode *x = rbtree_find(rb,n->key);
	if(x != NULL && equal(rb,n->key,x->key))
		return -1;
	n->color = RED;
	n->left = n->right = rb->nil;
	n->tree = rb;	
	if(x == NULL)
	{
		n->parent = rb->nil;
		rb->root = n;
	}
	else
	{
		n->parent = x;
		if(less(rb,n->key,x->key))
			x->left = n;
		else 
			x->right = n;
	}
	++rb->size;
	insert_fix_up(rb,n);
	return 0;
}

static inline rbnode *minimum(rbtree_t rb,rbnode *n)
{
	while(n->left != rb->nil)
		n = n->left;
	return n;
}

static inline rbnode *maxmum(rbtree_t rb,rbnode *n)
{
	while(n->right != rb->nil)
		n = n->right;
	return n;
}


static inline rbnode *successor(rbtree_t rb,rbnode *n)
{
	assert(rb);
	if(n->right != rb->nil)
		return minimum(rb,n->right);
	rbnode *y = n->parent;
	while(y != rb->nil && n == y->right)
	{
		n = y;
		y = y->parent;
	}
	return y;	
}

static inline rbnode *predecessor(rbtree_t rb,rbnode *n)
{
	assert(rb);
	if(n->left != rb->nil)
		return maxmum(rb,n->left);
	rbnode *y = n->parent;
	while(y != rb->nil && n == y->left)
	{
		n = y;
		y = y->parent;
	}
	return y;	
}

static inline rbnode *get_delete_node(rbtree_t rb,rbnode *n)
{
	if(n->left == rb->nil && n->right == rb->nil)
		return n;
	else if(n->right != rb->nil)
		return minimum(rb,n->right);
	else
		return maxmum(rb,n->left);
}

static void delete_fix_up(rbtree_t rb,rbnode *n)
{
	while(n != rb->root && n->color != RED)
	{
		rbnode *p = n->parent;
		if(n == p->left)
		{
			rbnode *w = p->right;
			if(w->color == RED)
			{
				w->color = BLACK;
				p->color = RED;
				rotate_left(rb,p);
				w = p->right;
			}
			if(w->left->color == BLACK && w->right->color == BLACK)
			{
				w->color = RED;
				n = p;
			}
			else
			{
				if(w->right->color == BLACK)
				{
					w->left->color == BLACK;
					w->color = RED;
					rotate_right(rb,w);
					w = p->right;
				}
				w->color = p->color;
				p->color = BLACK;
				w->right->color = BLACK;
				rotate_left(rb,p);
				n = rb->root;
			}
		}
		else
		{
			rbnode *w = p->left;
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

//将n从rbtree中剥离
static int8_t rb_delete(rbtree_t rb,rbnode *n)
{	
	rbnode *x = get_delete_node(rb,n);
	if(!x)
		return -1;
	/*将x从rbtree中剥离*/	
	rbnode *parent = x->parent;
	rbnode **link = (x == parent->left)? &(parent->left):&(parent->right);
	rbnode *z = rb->nil;	
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
	/*到这里x已经从rbtree中剥离,现在要用x替换n所在的位置
	* 把x放回rbtree中,将n剥离
	*/
	uint8_t x_old_color = x->color;
	if(n != x)
	{
		rbnode *n_left = n->left;
		rbnode *n_right = n->right;
		rbnode *n_parent = n->parent;
		if(n_left)
			n_left->parent = x;
		if(n_right)
			n_right->parent = x;
		if(n_parent)
		{
			if(n == n_parent->left)
				n_parent->left = x;
			else
				n_parent->right = x;
		}
		x->color = n->color;
	}
	if(n != rb->root && x_old_color == BLACK)
		delete_fix_up(rb,z);
	if(n == rb->root)
		rb->root = rb->nil;
	--rb->size;
	return 0;
}

int8_t rbtree_erase(rbnode *n)
{
	if(!n->tree)
		return -1;
	return rb_delete(n->tree,n);		
}

rbnode* rbtree_remove(rbtree_t rb,void *key)
{
	rbnode *n = rbtree_find(rb,key);
	if(n)
	{
		rbtree_erase(n);
		return n;
	}
	return NULL;
}

rbnode*  rbtree_first(rbtree_t rb)
{
	if(rb->size == 0)
		return NULL;
	return minimum(rb,rb->root);
}

rbnode*  rbtree_last(rbtree_t rb)
{
	if(rb->size == 0)
		return NULL;
	return maxmum(rb,rb->root);
}

rbnode*  rbnode_next(rbtree_t rb,rbnode *n)
{
	rbnode *succ = successor(rb,n);	
	if(succ == rb->nil)
		return NULL;
	return succ;
}

rbnode*  rbnode_pre(rbtree_t rb,rbnode*n)
{
	rbnode *presucc = predecessor(rb,n);
	if(presucc == rb->nil)
		return NULL;
	return presucc;	
}

int32_t check(rbtree_t rb,rbnode *n,int32_t level,int32_t black_level,int32_t *max_black_level,int32_t *max_level)
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

void rbtree_check_vaild(rbtree_t rb)
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