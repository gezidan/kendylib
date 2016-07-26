#include "trans_table.h"
#include "kstring.h"
#include "list.h"
#include "vector.h"
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 4096

string_t _readline(FILE *f)
{
   size_t spos = ftell(f);
   char line[MAX_LINE];
   uint32_t r = fread(line,1,MAX_LINE,f);
   string_t l = NULL;
   if(r == 0)
     return l;   
   int i = 0;
   for(; i < r; ++i)
   {
	  if(line[i] == '\n')			   
		break;
   }	
   if(i == r)
	 return l;
   line[i] = '\0';
   l = string_create(line);
   fseek(f,spos+i+1,SEEK_SET);
   return l;      	         					
}

static uint64_t _hash_func(void *arg)
{
	string_t str = *(string_t*)arg;
	return burtle_hash((uint8_t*)string_c_str(str),string_size(str),1);
}

static int32_t _hash_key_eq(void *l,void *r)
{
	string_t str_l = *(string_t*)l;
	string_t str_r = *(string_t*)r;
	return string_compare(str_l,str_r);
}

trans_table_t trans_table_create(FILE *trans_rule)
{
	hash_map_t h = hash_map_create(4096,sizeof(string_t),sizeof(uint32_t),_hash_func,_hash_key_eq);
	string_t l;
	int ti = 0;
	while(l =_readline(trans_rule))
	{
		vector_t v = vector_create(sizeof(string_t),0);		
		string_split(l,v,":,");
		
		int size = vector_size(v);
		if(size == 0)
		{
			printf("error\n");
			exit(0);
		}
		string_t *s = VECTOR_TO_ARRAY(string_t,v);
		string_t tbname = string_create(string_c_str(s[0]));
		//push table name
		HASH_MAP_INSERT(string_t,int,h,tbname,ti);
		int i = 1;
		for( ; i < size; ++i)
		{
			string_t colname = string_create(string_c_str(s[0]));
			colname = string_cat(colname,s[i]);
			HASH_MAP_INSERT(string_t,int,h,colname,i-1);
		}
		
		for(i = 0; i < size; ++i)
			string_destroy(&s[i]);
			
		vector_destroy(&v);
		string_destroy(&l);
		++ti;
	}
	
	trans_table_t tt = calloc(1,sizeof(*tt));
	tt->h = h;
	return tt;
}

int32_t trans_table_trans(trans_table_t tt,const char *tbname,const char *colname)
{
	char tmp[256];
	strcpy(tmp,tbname);
	if(colname)
		strcat(tmp,colname);
	string_t key = string_create(tmp);
	hash_map_iter it = HASH_MAP_FIND(string_t,tt->h,key);
	string_destroy(&key);
	if(!HASH_MAP_ITER_VAILD(it))
		return -1;
	return HASH_MAP_ITER_GET(int32_t,it);
}

/*
int main()
{
	FILE *f = fopen("test.txt","rb");
	trans_table_t tt = trans_table_create(f);
	//tablename1:1column1,1column2,1column3
	printf("%d ",trans_table_trans(tt,"tablename1",NULL));
	printf("%d ",trans_table_trans(tt,"tablename1","1column1"));
	printf("%d ",trans_table_trans(tt,"tablename1","1column2"));
	printf("%d ",trans_table_trans(tt,"tablename1","1column3"));
	printf("\n");

	printf("%d ",trans_table_trans(tt,"tablename2",NULL));
	printf("%d ",trans_table_trans(tt,"tablename2","2column1"));
	printf("%d ",trans_table_trans(tt,"tablename2","2column2"));
	printf("%d ",trans_table_trans(tt,"tablename2","2column3"));
	printf("\n");
	
	printf("%d ",trans_table_trans(tt,"tablename3",NULL));
	printf("%d ",trans_table_trans(tt,"tablename3","3column1"));
	printf("%d ",trans_table_trans(tt,"tablename3","3column2"));
	printf("%d ",trans_table_trans(tt,"tablename3","3column3"));
	printf("\n");
	
	return 0;
}*/

