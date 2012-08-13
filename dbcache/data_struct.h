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
#include "link_list.h"
enum
{
	DB_LIST = 1,
	DB_ARRAY,
};

typedef struct db_element
{
	struct refbase ref;
	int32_t hash_count;
	int8_t type;
}*db_element_t;

db_element_t db_element_acquire(db_element_t,db_element_t);
void db_element_release(db_element_t*);


//represent a db row
typedef struct db_array
{
	struct db_element base;
	int32_t     size;
	basetype_t* data; 
}*db_array_t;


db_array_t db_array_create(int32_t size);
db_array_t db_array_acquire(db_array_t,db_array_t);
void       db_array_clear(db_array_t);//clear the data
void       db_array_release(db_array_t*);


//get/set one element of the db row
basetype_t db_array_get(db_array_t,int32_t index);
void       db_array_set(db_array_t,int32_t index,basetype_t);

struct db_node
{
	list_node  next;
	db_array_t array;
};

//represent db row set
typedef struct db_list
{
	struct db_element base;
	struct link_list *l;
	
}*db_list_t;

db_list_t db_list_create();
db_list_t db_list_acquire(db_list_t,db_list_t);
void      db_list_release(db_list_t*);
int32_t   db_list_append(db_list_t,db_array_t);
int32_t   db_list_size(db_list_t);
int8_t    db_list_shrink(db_list_t,uint32_t maxtime);


#endif	
