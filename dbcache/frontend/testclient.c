#include "trans_table.h"
#include <stdio.h>
#include "global_table.h"
#include "SysTime.h"
#include "wpacket.h"
#include "rpacket.h"



db_element_t find(global_table_t gt,wpacket_t wpk)
{
	rpacket_t rpk = rpacket_create_by_wpacket(NULL,wpk);
	char key[4096];
	strcpy(key,rpacket_read_string(rpk));
	int16_t size = rpacket_read_uint16(rpk);
	int i = 0;
	for( ; i < size; ++i)
	{
	   char tmp[32];
	   snprintf(tmp,32,",%u",rpacket_read_uint32(rpk));
	   strcat(key,tmp);			
	}
	return global_table_find(gt,key,global_hash(key));
}

void del(global_table_t gt,wpacket_t wpk)
{
	rpacket_t rpk = rpacket_create_by_wpacket(NULL,wpk);
	char key[4096];
	strcpy(key,rpacket_read_string(rpk));
	int16_t size = rpacket_read_uint16(rpk);
	int i = 0;
	for( ; i < size; ++i)
	{
	   char tmp[32];
	   snprintf(tmp,32,",%u",rpacket_read_uint32(rpk));
	   strcat(key,tmp);			
	}
	global_table_remove(gt,key,global_hash(key));
}

int main()
{
	init_system_time(10);
	global_table_t gtb = global_table_create(1024);
	FILE *f = fopen("dbdefine.txt","rb");
	trans_table_t tt = trans_table_create(f);	
	{
		//insert 3 record
		db_array_t record1 = db_array_create(4);
		db_array_set(record1,0,basetype_create_int32(1001));
		db_array_set(record1,1,basetype_create_int32(100));
		db_array_set(record1,2,basetype_create_int32(12));
		db_array_set(record1,3,basetype_create_int32(15));
		global_table_add(gtb,"mission,chaid,missionid,1001,100",(db_element_t)record1,global_hash("mission,chaid,missionid,1001,100"));
		global_table_add(gtb,"mission,chaid,1001",(db_element_t)record1,global_hash("mission,chaid,1001"));
		db_array_release(&record1);
		
		record1 = db_array_create(4);
		db_array_set(record1,0,basetype_create_int32(1001));
		db_array_set(record1,1,basetype_create_int32(101));
		db_array_set(record1,2,basetype_create_int32(13));
		db_array_set(record1,3,basetype_create_int32(14));
		global_table_add(gtb,"mission,chaid,missionid,1001,101",(db_element_t)record1,global_hash("mission,chaid,missionid,1001,101"));
		global_table_add(gtb,"mission,chaid,1001",(db_element_t)record1,global_hash("mission,chaid,1001"));
		db_array_release(&record1);
		
		record1 = db_array_create(4);
		db_array_set(record1,0,basetype_create_int32(1001));
		db_array_set(record1,1,basetype_create_int32(102));
		db_array_set(record1,2,basetype_create_int32(18));
		db_array_set(record1,3,basetype_create_int32(17));
		global_table_add(gtb,"mission,chaid,1001",(db_element_t)record1,global_hash("mission,chaid,1001"));
		global_table_add(gtb,"mission,chaid,missionid,1001,102",(db_element_t)record1,global_hash("mission,chaid,missionid,1001,102"));
		db_array_release(&record1);		
		
		db_array_t record2 = db_array_create(5);
		db_array_set(record2,0,basetype_create_int32(1001));
		db_array_set(record2,1,basetype_create_int32(10));
		db_array_set(record2,2,basetype_create_int32(11));
		db_array_set(record2,3,basetype_create_int32(12));
		db_array_set(record2,4,basetype_create_int32(14));
		global_table_add(gtb,"cha_attr,chaid,1001",(db_element_t)record2,global_hash("cha_attr,chaid,1001"));
		db_array_release(&record2);
		
		db_array_t record3 = db_array_create(5);
		db_array_set(record3,0,basetype_create_int32(1001));
		db_array_set(record3,1,basetype_create_int32(1000));
		db_array_set(record3,2,basetype_create_int32(100));
		db_array_set(record3,3,basetype_create_int32(12));
		db_array_set(record3,4,basetype_create_int32(14));
		global_table_add(gtb,"cha_bag,chaid,1001",(db_element_t)record3,global_hash("cha_bag,chaid,1001"));
		db_array_release(&record3);
		 
	}
	{
		//test find mission
		wpacket_t select = wpacket_create(0,NULL,512,0);
		wpacket_write_string(select,"mission,chaid,missionid");
		wpacket_write_uint16(select,2);
		wpacket_write_uint32(select,1001);//index value	
		wpacket_write_uint32(select,100);//index value
					
		db_array_t ret = (db_array_t)find(gtb,select);
		if(ret)
		{
			printf("chaid:%d,",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"mission","chaid"))));
			printf("missionid:%d,",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"mission","missionid"))));
			printf("mission_attr1:%d,",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"mission","mission_attr1"))));
			printf("mission_attr2:%d\n",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"mission","mission_attr2"))));	
		}
		wpacket_destroy(&select);
		
		wpacket_t delete = wpacket_create(0,NULL,512,0);
		wpacket_write_string(delete,"mission,chaid,missionid");
		wpacket_write_uint16(delete,2);
		wpacket_write_uint32(delete,1001);//index value	
		wpacket_write_uint32(delete,100);//index value
		del(gtb,delete);
		wpacket_destroy(&delete);		
		
		select = wpacket_create(0,NULL,512,0);
		wpacket_write_string(select,"mission,chaid");
		wpacket_write_uint16(select,1);
		wpacket_write_uint32(select,1001);//index value	
		db_list_t record_set = (db_list_t)find(gtb,select);
		if(record_set)
		{
			printf("chaid == 1001 recordset\n");
			int i = 0;
			int size = db_list_size(record_set);
			list_node *cur = link_list_head(record_set->l);
			while(cur)
			{
				db_array_t a = ((struct db_node*)cur)->array;
				if(a->data)
				{
					printf("chaid:%d,",basetype_get_int32(db_array_get(a,trans_table_trans(tt,"mission","chaid"))));
					printf("missionid:%d,",basetype_get_int32(db_array_get(a,trans_table_trans(tt,"mission","missionid"))));
					printf("mission_attr1:%d,",basetype_get_int32(db_array_get(a,trans_table_trans(tt,"mission","mission_attr1"))));
					printf("mission_attr2:%d\n",basetype_get_int32(db_array_get(a,trans_table_trans(tt,"mission","mission_attr2"))));
				}
				cur = cur->next;
			}
			printf("-------------end-------------\n");
		}
		
	}
	{
		//test find cha_attr
		wpacket_t select = wpacket_create(0,NULL,512,0);
		wpacket_write_string(select,"cha_attr,chaid");
		wpacket_write_uint16(select,1);
		wpacket_write_uint32(select,1001);//index value	
					
		db_array_t ret = (db_array_t)find(gtb,select);
		if(ret)
		{
			printf("chaid:%d,",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"cha_attr","chaid"))));
			printf("attr1:%d,",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"cha_attr","attr1"))));
			printf("attr2:%d,",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"cha_attr","attr2"))));
			printf("attr3:%d,",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"cha_attr","attr3"))));
			printf("attr4:%d\n",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"cha_attr","attr4"))));		
		}
		wpacket_destroy(&select);		
	}
	{
		//test find cha_bag
		wpacket_t select = wpacket_create(0,NULL,512,0);
		wpacket_write_string(select,"cha_bag,chaid");
		wpacket_write_uint16(select,1);
		wpacket_write_uint32(select,1001);//index value	
					
		db_array_t ret = (db_array_t)find(gtb,select);
		if(ret)
		{
			printf("chaid:%d,",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"cha_bag","chaid"))));
			printf("money:%d,",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"cha_bag","money"))));
			printf("bagpos1:%d,",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"cha_bag","bagpos1"))));
			printf("bagpos2:%d,",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"cha_bag","bagpos2"))));
			printf("bagpos3:%d\n",basetype_get_int32(db_array_get(ret,trans_table_trans(tt,"cha_bag","bagpos3"))));		
		}
		wpacket_destroy(&select);		
	}
	printf("%d\n",global_table_size(gtb));	
	return 0;
	
		
}
