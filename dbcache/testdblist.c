#include <stdio.h>
#include "data_struct.h"

int main()
{
	
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
	
	
	db_list_shrink(l);
	
	
	
	
	
	return 0;
}
