#ifndef __RBTREE_H
#define __RBTREE_H

typedef struct rbtree *rbtree_t;

typedef struct{
	rbnode *parent;
	rbnode *left;
	rbnode *right;
	rbtree_t tree;
	uint16_t color;	
}rbnode;

typedef int8_t (*cmp_function)(void*,void*);

typedef struct{
	rbnode *root;
	rbnode *nil;
	uint32_t size;
	uint16_t key_size;
	uint16_t val_size;	
	cmp_function compare_function;
	rbnode  dummy;
}rbtree;



rbtree_t create_rbtree(int32_t k_size,int32_t v_size,cmp_function);
void     destroy_rbtree(rbtree_t *);
int8_t   rbtree_insert(rbtree_t,rbnode*);
rbnode*  rbtree_find(rbtree_t,void *key);
int8_t   rbtree_erase(rbtree_t,rbnode*);
rbnode*  rbtree_remove(rbtree_t,void *key);

#endif

