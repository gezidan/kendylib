#include <stdio.h>
#include "data_struct.h"
#include "SysTime.h"
int main()
{
	init_system_time(10);
	db_list_t l = db_list_create();
	
	db_array_t a1 = db_array_create(3);
	db_array_t a2 = db_array_create(3);
	db_array_t a3 = db_array_create(3);
	db_array_t a4 = db_array_create(3);
	
	db_array_set(a1,0,basetype_create_int32(10));
	db_array_set(a1,1,basetype_create_int32(11));
	db_array_set(a1,2,basetype_create_int32(12));
	
	db_array_set(a2,0,basetype_create_int32(20));
	db_array_set(a2,1,basetype_create_int32(21));
	db_array_set(a2,2,basetype_create_int32(22));
	
	db_array_set(a3,0,basetype_create_int32(30));
	db_array_set(a3,1,basetype_create_int32(31));
	db_array_set(a3,2,basetype_create_int32(32));
	
	db_array_set(a4,0,basetype_create_int32(40));
	db_array_set(a4,1,basetype_create_int32(41));
	db_array_set(a4,2,basetype_create_int32(42));
	
	db_list_append(l,a1);
	db_list_append(l,a2);
	db_list_append(l,a3);
	db_list_append(l,a4);
	
	db_array_clear(a1);
	db_array_clear(a4);
	
	int32_t i = 0;
	for( ; i < 3; ++i)
	{
		basetype_t b = db_array_get(a2,i);
		printf("%d\n",basetype_get_int32(b));
	}
	
	i = 0;
	for( ; i < 3; ++i)
	{
		basetype_t b = db_array_get(a3,i);
		printf("%d\n",basetype_get_int32(b));
	}
	
	db_array_release(&a1);
	db_array_release(&a2);
	db_array_release(&a3);
	db_array_release(&a4);
	
	db_list_shrink(l,100);
	
	db_list_release(&l);	
	return 0;
}
