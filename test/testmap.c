#include <stdio.h>
#include "util/map.h"
#include "util/RBtree.h"

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
}mmnode;

int main()
{
	mmnode ns[10];
	int i = 0;
	for( ; i < 10; ++i)
	{
		ns[i].key = i+1;
		ns[i].base.key = &ns[i].key;
	}

	rbtree_t rb = create_rbtree(_comp);
	for(i = 0; i < 10; ++i)
		rbtree_insert(rb,(rbnode*)&ns[i]);
	{
		mmnode *n = (mmnode*)rbtree_first(rb);
		while(n)
		{
			printf("%d\n",n->key);
			n = (mmnode*)rbnode_next((rbnode*)n);
		}
	}
	rbtree_check_vaild(rb);

	mmnode *succ = (mmnode*)rbtree_remove(rb,(void*)&ns[3].key);
	printf("%d\n",succ->key);
	rbtree_check_vaild(rb);
	{
		mmnode *n = (mmnode*)rbtree_first(rb);
		while(n)
		{
			printf("%d\n",n->key);
			n = (mmnode*)rbnode_next((rbnode*)n);
		}
	}

	{
		mmnode *n = (mmnode*)rbtree_last(rb);
		while(n)
		{
			printf("%d\n",n->key);
			n = (mmnode*)rbnode_pre((rbnode*)n);
		}
	}
/*
	map_t m = MAP_CREATE(int,int,_comp,NULL);
	MAP_INSERT(int,int,m,1,1);
	MAP_INSERT(int,int,m,2,2);
	MAP_INSERT(int,int,m,3,3);
	MAP_INSERT(int,int,m,4,4);
	MAP_INSERT(int,int,m,5,5);
	MAP_INSERT(int,int,m,6,6);
	MAP_INSERT(int,int,m,7,7);
	MAP_INSERT(int,int,m,8,8);
	MAP_INSERT(int,int,m,9,9);
	MAP_INSERT(int,int,m,10,10);
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
