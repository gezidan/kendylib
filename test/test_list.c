#include <stdio.h>
#include "util/list.h"

int main()
{
	list_t l = LIST_CREATE(int);
	LIST_PUSH_BACK(int,l,1);
	LIST_PUSH_BACK(int,l,2);
	LIST_PUSH_BACK(int,l,3);
	LIST_PUSH_BACK(int,l,4);
	LIST_PUSH_BACK(int,l,5);
	
	printf("----test iter--------\n");
	list_iter it = list_begin(l);
	list_iter end = list_end(l);
	
	for( ; !IT_EQ(it,end); IT_NEXT(it))
		printf("%d\n",IT_GET_VAL(int,it));
	printf("-----test find & erase 2-------\n");	
	it = LIST_FIND(int,l,2);
	it = list_erase(l,it);
	printf("the next of 2 is %d\n",IT_GET_VAL(int,it));
	it = list_begin(l);
	end = list_end(l);
	for( ; !IT_EQ(it,end); IT_NEXT(it))
		printf("%d\n",IT_GET_VAL(int,it));	
	printf("-----test remove 4-------\n");
	LIST_REMOVE(int,l,4);
	it = list_begin(l);
	end = list_end(l);
	for( ; !IT_EQ(it,end); IT_NEXT(it))
		printf("%d\n",IT_GET_VAL(int,it));
	printf("-----test reverse iter-------\n");	
	it = list_rbegin(l);
	end = list_rend(l);
	for( ; !IT_EQ(it,end); IT_NEXT(it))
		printf("%d\n",IT_GET_VAL(int,it));
	printf("-----test push front 100-------\n");
	LIST_PUSH_FRONT(int,l,100);	
	it = list_begin(l);
	end = list_end(l);
	for( ; !IT_EQ(it,end); IT_NEXT(it))
		printf("%d\n",IT_GET_VAL(int,it));		
	printf("end\n");
	printf("-----test front & back-------\n");
	printf("the front is:%d\n",LIST_FRONT(int,l));
	printf("the back is:%d\n",LIST_BACK(int,l));
	
	printf("-----test pop front & pop back-------\n");
	LIST_POP_BACK(int,l);
	LIST_POP_FRONT(int,l);
	printf("the front is:%d\n",LIST_FRONT(int,l));
	printf("the back is:%d\n",LIST_BACK(int,l));
	
	return 0;
}
