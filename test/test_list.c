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
	
	list_iter it = list_begin(l);
	list_iter end = list_end(l);
	
	for( ; !IT_EQ(it,end); it = IT_NEXT(list_iter,it))
		printf("%d\n",IT_GET_VAL(int,it));
	printf("------------\n");	
	it = LIST_FIND(int,l,2);
	it = list_erase(l,it);
	printf("%d\n",IT_GET_VAL(int,it));
	printf("------------\n");
	it = list_begin(l);
	end = list_end(l);
	for( ; !IT_EQ(it,end); it = IT_NEXT(list_iter,it))
		printf("%d\n",IT_GET_VAL(int,it));	
	printf("------------\n");
	LIST_REMOVE(int,l,4);
	it = list_begin(l);
	end = list_end(l);
	for( ; !IT_EQ(it,end); it = IT_NEXT(list_iter,it))
		printf("%d\n",IT_GET_VAL(int,it));
	printf("------------\n");	
	it = list_rbegin(l);
	end = list_rend(l);
	for( ; !IT_EQ(it,end); it = IT_NEXT(list_iter,it))
		printf("%d\n",IT_GET_VAL(int,it));			
		
	printf("end\n");
	return 0;
}
