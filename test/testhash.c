#include <stdio.h>
#include <stdlib.h>
#include "hash_map.h"
#include "SysTime.h"
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
	for( ; i < 10000001; ++i)
	{
		hash_map_iter it = HASH_MAP_INSERT(int32_t,int32_t,h,i,i);
		//printf("%d\n",HASH_MAP_ITER_GET(int32_t,it));
	}
	
	uint32_t tick = GetSystemMs();
	for(i=1; i < 10000001; ++i)
	{
		hash_map_iter it = HASH_MAP_FIND(int32_t,h,i);
		//printf("%d\n",HASH_MAP_ITER_GET(int32_t,it));
	}
	printf("%u\n",GetSystemMs()-tick);
	for(i=1; i < 10000001; ++i)
	{
		void *ret = HASH_MAP_REMOVE(int32_t,h,i);
		if(ret == NULL)
		{
			printf("error\n");
			exit(0);
		}
		//else
		//	printf("%d\n",*(int32_t*)ret);
	}
	
	printf("size:%u\n",hash_map_size(h));
	
	
	
	
	return 0;
	
	
	
};
