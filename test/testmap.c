#include <stdio.h>
#include "util/map.h"

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

int main()
{
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
	
	
	return 0;
}