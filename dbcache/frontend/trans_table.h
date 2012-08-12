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

//transfrom string to integer
#include "hash_table.h"
#include <stdio.h>

typedef struct trans_table
{
	
}*trans_table_t;


/*
 * rule file format
 * tablename1:1column1,1column2,1column3
 * tablename2:2column1,2column2,2column3
 * tablename3:3column1,3column2,3column3
 * 
 * call trans_table_trans("tablename1",NULL) return 0
 * call trans_table_trans("tablename2",NULL) return 1
 * call trans_table_trans("tablename3","3column3") return 2,this return value could used as 
 * index in db_array_t to fecth the column value.
 */

trans_table_t trans_table_create(FILE *trans_rule); 
void trans_table_destroy(trans_table_t*);

/* para1 is the table name,para2 is the column name
 * if just want to transfrom the table name,let para2 be NULL
 */ 
int32_t trans_table_trans(const char*,const char*); 


