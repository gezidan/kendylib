#include <stdio.h>
//#include "util/map.h"

#include "util/rbtree.h"

int32_t _comp(void *_a,void *_b)
{
	int *a = (int*)_a;
	int *b = (int*)_b;
	if(*a == *b)
		return 0;
	else if(*a > *b)
		return 1;
	else 
		return -1;
}

typedef struct mmnode
{
	rbnode base;
	int key;
	int val;
}mmnode;

int main()
{
	
	mmnode n1;
	n1.key = n1.val = 1;
	n1.base.key = &n1.key;
	n1.base.val = &n1.val;
	
	mmnode n2;
	n2.key = n2.val = 2;
	n2.base.key = &n2.key;
	n2.base.val = &n2.val;

	mmnode n3;
	n3.key = n3.val = 3;
	n3.base.key = &n3.key;
	n3.base.val = &n3.val;

	mmnode n4;
	n4.key = n4.val = 4;
	n4.base.key = &n4.key;
	n4.base.val = &n4.val;

	mmnode n5;
	n5.key = n5.val = 5;
	n5.base.key = &n5.key;
	n5.base.val = &n5.val;

	mmnode n6;
	n6.key = n6.val = 6;
	n6.base.key = &n6.key;
	n6.base.val = &n6.val;

	mmnode n7;
	n7.key = n7.val = 7;
	n7.base.key = &n7.key;
	n7.base.val = &n7.val;

	mmnode n8;
	n8.key = n8.val = 8;
	n8.base.key = &n8.key;
	n8.base.val = &n8.val;

	mmnode n9;
	n9.key = n9.val = 9;
	n9.base.key = &n9.key;
	n9.base.val = &n9.val;	
	
	mmnode n10;
	n10.key = n10.val = 10;
	n10.base.key = &n10.key;
	n10.base.val = &n10.val;
	
	rbtree_t rb = create_rbtree(_comp);
	rbtree_insert(rb,(rbnode*)&n1);
	rbtree_insert(rb,(rbnode*)&n2);
	rbtree_insert(rb,(rbnode*)&n3);
	rbtree_insert(rb,(rbnode*)&n4);
	rbtree_insert(rb,(rbnode*)&n5);
	rbtree_insert(rb,(rbnode*)&n6);
	rbtree_insert(rb,(rbnode*)&n7);
	rbtree_insert(rb,(rbnode*)&n8);
	rbtree_insert(rb,(rbnode*)&n9);
	rbtree_insert(rb,(rbnode*)&n10);
	{
		mmnode *n = (mmnode*)rbtree_first(rb);
		while(n)
		{
			printf("%d\n",n->key);
			n = (mmnode*)rbnode_next(rb,(rbnode*)n);
		}
	}	
	rbtree_check_vaild(rb);
	
	mmnode *succ = (mmnode*)rbtree_remove(rb,(void*)&n3.key);
	printf("%d\n",succ->key);
	rbtree_check_vaild(rb);
	{
		mmnode *n = (mmnode*)rbtree_first(rb);
		while(n)
		{
			printf("%d\n",n->key);
			n = (mmnode*)rbnode_next(rb,(rbnode*)n);
		}
	}

	{
		mmnode *n = (mmnode*)rbtree_last(rb);
		while(n)
		{
			printf("%d\n",n->key);
			n = (mmnode*)rbnode_pre(rb,(rbnode*)n);
		}
	}		
	
/*
	map_t m = MAP_CREATE(int,int,_comp,NULL);
	MAP_INSERT(int,int,m,1,1);
	MAP_INSERT(int,int,m,2,2);
	MAP_INSERT(int,int,m,3,3);
	MAP_INSERT(int,int,m,4,4);
	MAP_INSERT(int,int,m,5,5);
	printf("------test iter------\n");	
	map_iter it = map_begin(m);
	map_iter end = map_end(m);
	for( ; !IT_EQ(it,end); IT_NEXT(it))
		printf("%d\n",IT_GET_VAL(int,it));
	printf("------test remove 4------\n");	
	MAP_REMOVE(int,m,4);
	it = map_begin(m);
	end = map_end(m);
	for( ; !IT_EQ(it,end); IT_NEXT(it))
		printf("%d\n",IT_GET_VAL(int,it));	
*/	
	
	return 0;
}