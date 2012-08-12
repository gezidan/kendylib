#include <stdio.h>
#include "global_table.h"
#include "SysTime.h"

int main()
{
	init_system_time(10);
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
	global_table_add(gtb,"kenny2",(db_element_t)a2);
	global_table_add(gtb,"kenny3",(db_element_t)a3);
	global_table_add(gtb,"kenny4",(db_element_t)a4);
		
		
	//test search	
	db_list_t l = (db_list_t)global_table_find(gtb,"kenny");
	
	printf("the row size of kenny(a db_list_t):%d\n",db_list_size(l));
	
	printf("element of a1:key(kenny1):");
	db_array_t _a = (db_array_t)global_table_find(gtb,"kenny1");
	int i = 0;
	for( ; i < 3; ++i)
	{
		basetype_t b = db_array_get(_a,i);
		printf("%d ",basetype_get_int32(b));
	}
	printf("\n");
	
	printf("element of a2:key(kenny2):");
	_a = (db_array_t)global_table_find(gtb,"kenny2");
	i = 0;
	for( ; i < 3; ++i)
	{
		basetype_t b = db_array_get(_a,i);
		printf("%d ",basetype_get_int32(b));
	}
	
	printf("\n");
	
	printf("element of a3:key(kenny3):");
	_a = (db_array_t)global_table_find(gtb,"kenny3");
	i = 0;
	for( ; i < 3; ++i)
	{
		basetype_t b = db_array_get(_a,i);
		printf("%d ",basetype_get_int32(b));
	}
	
	printf("\n");
	
	printf("element of a4:key(kenny4):");
	_a = (db_array_t)global_table_find(gtb,"kenny4");
	i = 0;
	for( ; i < 3; ++i)
	{
		basetype_t b = db_array_get(_a,i);
		printf("%d ",basetype_get_int32(b));
	}
	
	printf("\n");
	
	db_array_release(&a4);
	global_table_remove(gtb,"kenny4");
	/* shrink will cause the refcount of a4 reduce to zero,
	 * then a4 will be destroyed
	*/
	db_list_shrink(l,100);
	
	printf("the row size of kenny(a db_list_t),after remove and shrink: %d\n",db_list_size(l));		
	
	db_array_release(&a1);
	db_array_release(&a2);
	db_array_release(&a3);
	
	
	
	printf("destroy global table,this will cause all element destroyed\n");
	global_table_destroy(&gtb);
	
	return 0;
}
