/*	
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _DATA_STRUCT_H
#define _DATA_STRUCT_H
#include <stdint.h>
#include "refbase.h"
#include "dbtype.h"
enum
{
	DB_LIST = 1,
	DB_ARRAY,
};

typedef struct db_element
{
	struct refbase ref;
	int8_t type;
}*db_element_t;

typedef struct db_array
{
	struct db_element base;
	int32_t     size;
	basetype_t* data; 
}*db_array_t;


db_array_t db_array_create(int32_t size);
void       db_array_clear(db_array_t);//clear the data
void       db_array_release(db_array_t*);
basetype_t db_array_get(db_array_t,int32_t index);
void       db_array_set(db_array_t,int32_t index,basetype_t);
struct db_node
{
	struct db_node *next;
	db_array_t array;
};

typedef struct db_list
{
	struct db_element base;
	int32_t size;
	struct db_node *head;
	struct db_node *tail;
	
}*db_list_t;

db_list_t db_list_create();
void      db_list_destroy(db_list_t*);
int32_t   db_list_append(db_list_t,db_array_t);
int32_t   db_list_size(db_list_t);
int32_t   db_list_shrink(db_list_t);

#endif	
