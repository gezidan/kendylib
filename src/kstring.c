#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "vector.h"
#include "kstring.h"
#include "allocator.h"

typedef struct string
{
	uint32_t buf_size;
	char *str;
}* string_t;


string_t string_create(const char*str)
{
	string_t s = ALLOC(0,sizeof(*s));
	if(!s)
		return 0;
	if(!str)
	{
		s->str = 0;
		s->buf_size = 0;
	}
	else
	{
		s->buf_size = strlen(str)+1;
		s->str = ALLOC(0,s->buf_size);
		if(!s->str)
		{
			FREE(0,s);
			return 0;
		}
		strncpy(s->str,str,s->buf_size);
	}
	return s;
}

string_t string_create_n(const char*str,uint32_t n)
{
	string_t s = ALLOC(0,sizeof(*s));
	if(!s)
		return 0;
	if(!str)
	{
		s->str = 0;
		s->buf_size = 0;
	}
	else
	{
		s->buf_size = n+1;
		s->str = ALLOC(0,s->buf_size);
		if(!s->str)
		{
			FREE(0,s);
			return 0;
		}
		strncpy(s->str,str,n);
		s->str[n] = '\0';
	}
	return s;
}

void   string_destroy(string_t *s)
{
	assert(s);
	assert(*s);
	if((*s)->str)
		FREE(0,(*s)->str);
	FREE(0,*s);
	(*s) = 0;
}

inline uint32_t string_size(string_t s)
{
	assert(s);
	if(!s->str)
		return 0;
	return strlen(s->str);
}

inline const char *string_c_str(string_t s)
{
	assert(s);
	return s->str;
}


string_t string_assign_cstr(string_t a,const char *b)
{
	assert(a);
	assert(b);
	uint32_t len1 = string_size(a);
	uint32_t len2 = strlen(b);	
	if(!a->str || len1 < len2)
	{
		a->str = ALLOC(0,len2+1);
		if(!a->str)
		{
			return a;
		}
		a->buf_size = len2+1;
	}
	
	strncpy(a->str,b,len2+1);
	return a;	
}


string_t string_assign(string_t a,string_t b)
{
	assert(a);
	assert(b);
	if(!b->str)
		return a;
	return string_assign_cstr(a,b->str);
}

string_t string_cat_cstr(string_t a,const char *b)
{
	assert(a);
	assert(b);
	uint32_t len1 = string_size(a);
	uint32_t len2 = strlen(b);
	uint32_t len3 = len1+len2+1;
	if(!a->str)
	{
		a->str = ALLOC(0,len2+1);
		strncpy(a->str,b,len2+1);
		return a;
	}
	
	if(a->buf_size >= len3)
	{
		strcat(a->str,b);
	}
	else
	{
		char *buf = realloc(a->str,len3);
		if(!buf)
		{
			buf = ALLOC(0,len3);		
			if(!buf)
				return a;
			else
				strncpy(buf,a->str,len1+1);
		}
		a->str = buf;
		a->buf_size = len3;
		strcat(a->str,b);	
	}
	return a;	
}

string_t string_cat(string_t a,string_t b)
{
	assert(a);
	assert(b);
	if(!b->str)
		return a;
	return string_cat_cstr(a,b->str);
}

const char* string_find_cstr(string_t a,const char *b)
{
	assert(a);
	assert(b);
	if(!a->str)
		return 0;
	return strstr(a->str,b);	
}

const char* string_find(string_t a,string_t b)
{	
	assert(a);
	assert(b);
	if(!b->str)
		return 0;
	return string_find_cstr(a,b->str);
}

void string_split(string_t s,struct vector *v,const char *delimiters)
{
	assert(s);
	assert(v);
	assert(delimiters);
	if(!s->str)
		return;
	
	uint32_t size_d = strlen(delimiters);
	uint32_t i;
	if(size_d == 0)
		return;
	
	char *b = s->str;
	char *cur = b;
	while(*cur != '\0')
	{
		++cur;
		for(i = 0; i < size_d;++i)
		{
			if(*cur == delimiters[i])
			{
				string_t _s = string_create_n(b,cur-b);
				VECTOR_PUSH_BACK(string_t,v,_s);
				++cur;
				b = cur;
				break;
			}
		}
	}
	string_t _s = string_create_n(b,cur-b);
	VECTOR_PUSH_BACK(string_t,v,_s);	
}

int32_t string_compare(string_t l,string_t r)
{
	return strcmp(l->str,r->str);
}
