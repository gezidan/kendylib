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
#include "util/refbase.h"
#include "dbtype.h"
#include "util/link_list.h"

/*
* 内存数据库value支持的两个容器类型
*/

typedef struct db_array
{
	basetype base;
	int32_t size;
}*db_array_t;


basetype_t db_array_create(int32_t size);
void       db_array_clear(db_array_t);//clear the data


basetype_t db_array_get(db_array_t,int32_t index);
void       db_array_set(db_array_t,int32_t index,basetype_t);

struct db_node
{
	list_node  next;
	basetype_t element;
};

typedef struct db_list
{
	basetype base;
	int32_t size;
}*db_list_t;

basetype_t db_list_create();
int32_t    db_list_append(db_list_t,basetype_t);
basetype_t db_list_pop(db_list_t);
int32_t    db_list_size(db_list_t);
int8_t     db_list_shrink(db_list_t,uint32_t maxtime);

#endif	
