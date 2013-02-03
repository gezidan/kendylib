#ifdef _IS_OK_
#include "_rbtree.h"

#define RED 1
#define BLACK 2

rbtree_t create_rbtree(int32_t k_size,int32_t v_size,cmp_function compare_function)
{
	rbtree_t rb = malloc(sizeof(*rb));
	if(rb)
	{
		rb->size = 0;
		rb->dummy.color = BLACK;
		rb->nil = &rb->dummy;
		rb->nil->tree = rb;
		rb->root = rb->nil;
		rb->key_size = k_size;
		rb->val_size = v_size;
	}
	return rb;
}

void     destroy_rbtree(rbtree_t *rb)
{
	
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
				//ÊåÊå½ÚµãÊÇºìÉ«£¬Ö´ÐÐÒ»´ÎÑÕÉ«·­×ª
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
				//ÊåÊå½ÚµãÊÇºìÉ«£¬Ö´ÐÐÒ»´ÎÑÕÉ«·­×ª
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
		return rb->nil;
	rbnode *cur = rb->root;
	rbnode *pre;
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

int8_t rbtree_insert(rbtree_t rb,rbnode *n)
{
	assert(rb);	
	rbnode *x = find(rb,key);
	if(x != rb->nil && equal(rb,n->key,x->key))
		return -1;
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
		if(less(rb,n->key,x->key))
			x->left = n;
		else 
			x->right = n;
	}
	++rb->size;
	insert_fix_up(rb,n);
	return 0;
}

inline static rbnode *minimum(rbtree_t rb,rbnode *n)
{
	while(n->left != rb->nil)
		n = n->left;
	return n;
}

inline static rbnode *maxmum(rbtree_t rb,rbnode *n)
{
	while(n->right != rb->nil)
		n = n->right;
	return n;
}


static rbnode *successor(rbtree_t rb,rbnode *n)
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

//»ñµÃnµÄÇ°Çý½Úµã,Îª¸ù½ÚµãµÄÇé¿öÏÂ·µ»Ønil
static rbnode *predecessor(rbtree_t rb,rbnode *n)
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

static rbnode *get_delete_node(rbtree_t rb,rbnode *n)
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
			if(w->color == RED)//ÐÖµÜÎªºì
			{
				w->color = BLACK;
				p->color = RED;
				rotate_left(rb,p);
				w = p->right;
			}
			//ÐÖµÜÎªºÚ
			if(w->left->color == BLACK && w->right->color == BLACK)
			{
				//ÐÖµÜµÄÁ½¶ù×ÓÎªºÚ
				w->color = RED;
				n = p;
			}
			else
			{
				if(w->right->color == BLACK)
				{
					//ÐÖµÜµÄÓÒº¢×ÓÎªºÚ
					w->left->color == BLACK;
					w->color = RED;
					rotate_right(rb,w);
					w = p->right;
				}
				//ÐÖµÜµÄÓÒº¢×ÓÎªºì
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
/*
static rbnode* rb_delete(rbtree_t rb,rbnode *n,rbnode **succ)
{	
	rbnode *x = get_delete_node(rb,n);
	if(succ)
	{
		if(x == n || less(rb,x->key,n->key))
			*succ = successor(rb,x);
		else
			*succ = n;
	}
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
	if(n != x)
	{
		//¿½±´Êý¾Ý
		memcpy(n->key,x->key,rb->key_size;
		memcpy(n->value,x->value,rb->val_size;
	}
	if(x != rb->root && x->color == BLACK)
		delete_fix_up(rb,z);
	if(x == rb->root)
		rb->root = rb->nil;		
	--rb->size;
	return x;
}

int8_t   rbtree_erase(rbtree_t rb,rbnode *n)
{
}

rbnode*  rbtree_remove(void *key)
{
	
}
*/
#endif
