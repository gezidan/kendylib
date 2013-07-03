#include <stdio.h>
#include "global_table.h"
#include "SysTime.h"

int main()
{
	init_system_time(10);
	global_table_t gtb = global_table_create(65536);
	
	char key[64];
	uint32_t now = GetSystemMs();
	int32_t i = 0;
	for( ; i < 1000000; ++i)
	{
		basetype_t a = basetype_create_int32(i);
		snprintf(key,64,"test%d",i);
		a = global_table_insert(gtb,key,a,global_hash(key));
		if(!a)
			printf("error 1\n");
		basetype_release(&a);		
	}
	now = GetSystemMs() - now;
	printf("%u\n",now);
	now = GetSystemMs();
	int j = 0;
	for( ; j < 2; ++j)
		for(i=0 ; i < 1000000; ++i)
		{
			basetype_t a;
			snprintf(key,64,"test%d",i);
			a = global_table_find(gtb,key,global_hash(key));
			if(!a)
				printf("error 2\n");
			if(i != basetype_get_int32(a))
				printf("error 3\n");
			basetype_release(&a);		
		}
	now = GetSystemMs() - now;
	printf("%u\n",now);
	printf("table_size:%u\n",global_table_size(gtb));
	now = GetSystemMs();
	for(i=0; i < 1000000; ++i)
	{
		snprintf(key,64,"test%d",i);
		basetype_t a = global_table_remove(gtb,key,global_hash(key));
		if(!a)
			printf("error 1\n");
		basetype_release(&a);		
	}
	now = GetSystemMs() - now;
	printf("%u\n",now);	
	printf("table_size:%u\n",global_table_size(gtb));
	
	
	{
		printf("test1\n");
		//测试简单类型
		basetype_t a = basetype_create_int32(10);
		
		a = global_table_insert(gtb,"a",a,global_hash("a"));
		if(!a)
			printf("error 1\n");
		basetype_release(&a);	
			
		a = global_table_remove(gtb,"a",global_hash("a"));
		if(!a)
			printf("error 2\n");
		printf("%d\n",basetype_get_int32(a));	
		basetype_release(&a);	
	}
	
	{
		printf("test2\n");
		//测试array类型
		basetype_t _array = db_array_create(10);
		int i = 0;
		for( ; i < 10; ++i)
		{
			basetype_t a = basetype_create_int32(i);
			db_array_set((db_array_t)_array,i,a);
			basetype_release(&a);
		}
		
		_array = global_table_insert(gtb,"a",_array,global_hash("a"));
		if(!_array)
			printf("error 1\n");
		basetype_release(&_array);	
			
		_array = global_table_remove(gtb,"a",global_hash("a"));
		if(!_array)
			printf("error 2\n");
		i = 0;
		for( ; i < 10; ++i)
		{
			basetype_t a = db_array_get((db_array_t)_array,i);
			printf("%d\n",basetype_get_int32(a));
			basetype_release(&a);
		}
		basetype_release(&_array);	
	}
	
	{
		printf("test3\n");
		//测试list类型
		basetype_t _list = db_list_create();
		int i = 0;
		for( ; i < 10; ++i)
		{
			basetype_t a = basetype_create_int32(i);
			db_list_append((db_list_t)_list,a);
			basetype_release(&a);
		}
		_list = global_table_insert(gtb,"a",_list,global_hash("a"));
		if(!_list)
			printf("error 1\n");
		basetype_release(&_list);	
			
		_list = global_table_remove(gtb,"a",global_hash("a"));
		if(!_list)
			printf("error 2\n");
		basetype_t a;
		while(a = db_list_pop((db_list_t)_list))
		{
			printf("%d\n",basetype_get_int32(a));
			basetype_release(&a);
		}
		basetype_release(&_list);	
	}
	global_table_destroy(&gtb);
	
	return 0;
}