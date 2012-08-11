#include <stdio.h>
#include "global_table.h"


int main()
{
	global_table_t gtb = global_table_create(1024);
	
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
	
	global_table_add(gtb,"kenny",(db_element_t)a1);
	global_table_add(gtb,"kenny",(db_element_t)a2);
	global_table_add(gtb,"kenny",(db_element_t)a3);
	global_table_add(gtb,"kenny",(db_element_t)a4);
	global_table_add(gtb,"kenny1",(db_element_t)a1);
	
	db_array_release(&a1);
	db_array_release(&a2);
	db_array_release(&a3);
	db_array_release(&a4);
	
	
	db_list_t l = (db_list_t)global_table_find(gtb,"kenny");
	
	printf("%d\n",db_list_size(l));
	
	//global_table_remove(gtb,"kenny");
	
	global_table_destroy(&gtb);
	
	return 0;
}
