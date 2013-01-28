#include <stdio.h>
#include <stdlib.h>
#include "util/hash_map.h"
int32_t _hash_key_eq_(void *l,void *r)
{
	if(*(int32_t*)r == *(int32_t*)l)
		return 0;
	return -1;
}

uint64_t _hash_func_(void* key)
{
	return burtle_hash(key,sizeof(int32_t),1);
}

int main()
{
	
	hash_map_t h = hash_map_create(4096,sizeof(int32_t),sizeof(int32_t),_hash_func_,_hash_key_eq_);
	int32_t i = 1;
	for( ; i < 10; ++i)
	{
		hash_map_iter it = HASH_MAP_INSERT(int32_t,int32_t,h,i,i);
		printf("%d\n",IT_GET_VAL(int32_t,it));
	}	
	printf("----------------------\n");
	{
		hash_map_iter it = HASH_MAP_FIND(int32_t,h,5);
		hash_map_iter end = hash_map_end(h);
		if(!IT_EQ(it,end))
			printf("%d\n",IT_GET_VAL(int32_t,it));
	}
	printf("----------------------\n");	
	{
		hash_map_iter it = HASH_MAP_FIND(int32_t,h,100);
		hash_map_iter end = hash_map_end(h);
		if(!IT_EQ(it,end))
			printf("%d\n",IT_GET_VAL(int32_t,it));
		else 
			printf("can't find 100\n");
	}
	printf("----------------------\n");
	HASH_MAP_REMOVE(int32_t,h,5);
	{
		hash_map_iter it = hash_map_begin(h);
		hash_map_iter end = hash_map_end(h);
		for( ; !IT_EQ(it,end); it = IT_NEXT(hash_map_iter,it))
			printf("%d\n",IT_GET_VAL(int32_t,it));	
	}	
	return 0;
};
