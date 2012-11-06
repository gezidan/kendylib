#include "dbtype.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common_define.h"

static void basetype_destroy(void *arg)
{
	basetype_t b = (basetype_t)arg;
	free(b->data);
	free(b);
	//printf("basetype destroy\n");
} 

#define BASE_TYPE_CREATE(TYPE,INIT)\
({basetype_t b = (basetype_t)calloc(1,sizeof(*b));\
  b->data = (void*)calloc(1,sizeof(TYPE));\
  *(TYPE*)b->data = INIT;\
  b->ref.destroyer = basetype_destroy;\
  b->ref.mt = SINGLE_THREAD;\
  b->ref.refcount = 1;\
  b;})
  
basetype_t basetype_create_int8(int8_t init)
{
	basetype_t b = BASE_TYPE_CREATE(int8_t,init);
	b->type = DB_INT8;
	return b;		
}

basetype_t basetype_create_int16(int16_t init)
{
	basetype_t b = BASE_TYPE_CREATE(int16_t,init);
	b->type = DB_INT16;
	return b;		
}

basetype_t basetype_create_int32(int32_t init)
{
	basetype_t b = BASE_TYPE_CREATE(int32_t,init);
	b->type = DB_INT32;
	return b;		
}

basetype_t basetype_create_int64(int64_t init)
{
	basetype_t b = BASE_TYPE_CREATE(int64_t,init);
	b->type = DB_INT64;
	return b;			
}

basetype_t basetype_create_double(double init)
{
	basetype_t b = BASE_TYPE_CREATE(double,init);
	b->type = DB_DOUBLE;
	return b;			
}

basetype_t basetype_create_str(const char *init)
{
	if(init == NULL)
		return NULL;
	struct db_type_string *b = calloc(1,sizeof(*b));
	int32_t s = strlen(init)+1;
	b->base.data = calloc(1,s);
	strcpy((char *)b->base.data,init);
	b->size = s;
	return (basetype_t)b;
}

basetype_t basetype_create_bin(void *init,int32_t size)
{
	if(init == NULL || size <= 0)
		return NULL;
	struct db_type_binary *b = calloc(1,sizeof(*b));
	b->size = size;
	b->base.data = calloc(1,size);
	memcpy(b->base.data,init,size);
	return (basetype_t)b;			
}


#define BASE_TYPE_GET(TYPE,B)\
({TYPE ret = *(TYPE*)B->data;ret;})

int8_t      basetype_get_int8(basetype_t b)
{
	if(b == NULL)
		return 0;
	return BASE_TYPE_GET(int8_t,b);
}

int16_t     basetype_get_int16(basetype_t b)
{
	if(b == NULL)
		return 0;
	return BASE_TYPE_GET(int16_t,b);	
}
int32_t     basetype_get_int32(basetype_t b)
{
	if(b == NULL)
		return 0;
	return BASE_TYPE_GET(int32_t,b);	
}

int64_t     basetype_get_int64(basetype_t b)
{
	if(b == NULL)
		return 0;
	return BASE_TYPE_GET(int64_t,b);	
	
}
double      basetype_get_double(basetype_t b)
{
	if(b == NULL)
		return 0;
	return BASE_TYPE_GET(double,b);
}

const char *basetype_get_str(basetype_t b)
{
	if(b == NULL || b->type != DB_STRING)
		return NULL;
	return BASE_TYPE_GET(const char*,b);	
}

void*      basetype_get_bin(basetype_t b,int32_t *size)
{
	if(b == NULL || b->type != DB_BINARY)
		return NULL;
	*size = ((struct db_type_binary*)b)->size; 
	return BASE_TYPE_GET(void*,b);
}

#define BASE_TYPE_SET(TYPE,B,V)\
({if(b)*(TYPE*)B->data = V;})


void basetype_set_int8(basetype_t b,int8_t v)
{
	BASE_TYPE_SET(int8_t,b,v);
}

void basetype_set_int16(basetype_t b,int16_t v)
{
	BASE_TYPE_SET(int16_t,b,v);
}


void basetype_set_int32(basetype_t b,int32_t v)
{
	BASE_TYPE_SET(int32_t,b,v);
}

void basetype_set_int64(basetype_t b,int64_t v)
{
	BASE_TYPE_SET(int64_t,b,v);
}

void basetype_set_double(basetype_t b,double v)
{
	BASE_TYPE_SET(double,b,v);
}


void basetype_set_str(basetype_t b,const char *str)
{
	if(!b || !str || !b->type == DB_STRING)
		return;
	struct db_type_string *_b = (struct db_type_string*)b;
	int32_t len = strlen(str) + 1;
	if(_b->size >= len)
		strcpy(_b->base.data,str);
	else
	{
		free(_b->base.data);
		_b->base.data = calloc(1,len);
		strcpy(_b->base.data,str);
	}
}

void basetype_set_bin(basetype_t b,void *ptr,int32_t size)
{
	if(!b || !ptr || !b->type == DB_BINARY || size <= 0)
		return;
	struct db_type_binary *_b = (struct db_type_binary*)b;
	if(_b->size >= size)
		memcpy(_b->base.data,ptr,size);
	else
	{
		free(_b->base.data);
		_b->base.data = calloc(1,size);
		memcpy(_b->base.data,ptr,size);
	}
	
}

basetype_t  basetype_acquire(basetype_t b1,basetype_t b2)
{
	if(b1 == b2)
		return b1;	
	if(b2)
		ref_increase((struct refbase*)b2);
	if(b1)
		basetype_release(&b1);
	return b2;
}

void basetype_release(basetype_t *b)
{
	ref_decrease((struct refbase*)*b);
	*b = NULL;
}

 
