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
#ifndef _MAP_H
#define _MAP_H
#include <stdint.h>
typedef struct map_iter
{
	void *node;
	struct map_iter (*next)(struct map_iter*);
	void (*get_first)(struct map_iter,void*);
	void (*get_second)(struct map_iter,void*);
	void (*set_second)(struct map_iter,void*);
}map_iter;


extern int32_t map_iter_equal(map_iter,map_iter);

#ifndef IT_MAP_EQUAL
#define IT_MAP_EQUAL(IT1,IT2)\
	map_iter_equal(IT1,IT2)
#endif

#ifndef IT_MAP_GETFIRST
#define IT_MAP_GETFIRST(TYPE,ITER)\
   ({ TYPE __result;\
       do ITER.get_first(ITER,&__result);\
       while(0);\
       __result;})
#endif

#ifndef IT_MAP_GETSECOND
#define IT_MAP_GETSECOND(TYPE,ITER)\
   ({ TYPE __result;\
       do ITER.get_second(ITER,&__result);\
       while(0);\
       __result;})
#endif

#ifndef IT_MAP_SETSECOND
#define IT_MAP_SETSECOND(TYPE,ITER,VAL)\
	{TYPE __val=VAL;ITER.set_second(ITER,&__val);}
#endif

#ifndef IT_MAP_NEXT
#define IT_MAP_NEXT(ITER)\
	ITER.next(&ITER)
#endif

//能作为map存储容器的类型都必须实现的接口
struct interface_map_container
{
	map_iter (*insert)(struct interface_map_container*,void*,void*);
	map_iter (*erase)(struct interface_map_container*,map_iter);
	void     (*remove)(struct interface_map_container* rb,void*);
	map_iter (*find)(struct interface_map_container* rb,void *);
	map_iter (*begin)(struct interface_map_container* rb);
	map_iter (*end)(struct interface_map_container* rb);
	int32_t  (*size)(struct interface_map_container*);
	int32_t  (*empty)(struct interface_map_container*);
	void     (*destroy)(struct interface_map_container**);
};

//相等返回0,小于返回-1,大于返回1
typedef int32_t (*comp)(void*,void*);
typedef struct map *map_t;

//创建一个map,如果最后一个参数传0,则默认使用红黑树
map_t    map_create(uint16_t key_size,uint16_t val_size,comp _comp,struct interface_map_container*);
void     map_destroy(map_t*);

inline extern map_iter map_insert(map_t,void*,void*);
inline extern map_iter map_erase(map_t,map_iter);
inline extern void     map_remove(map_t,void*);
inline extern map_iter map_find(map_t,void *);
inline extern map_iter map_begin(map_t);
inline extern map_iter map_end(map_t);
inline extern int32_t  map_size(map_t);
inline extern int32_t  map_empty(map_t);

#ifndef MAP_CREATE
#define MAP_CREATE(KEY_TYPE,VAL_TYPE,COMP,CONTAINER)\
	map_create(sizeof(KEY_TYPE),sizeof(VAL_TYPE),COMP,CONTAINER)
#endif

#ifndef MAP_INSERT
#define MAP_INSERT(KEY_TYPE,VAL_TYPE,MAP,KEY,VAL)\
	({  map_iter it;KEY_TYPE __key = KEY;VAL_TYPE __val = VAL;\
       do it = map_insert(MAP,&__key,&__val);\
       while(0);\
       it;})		
#endif

#ifndef MAP_REMOVE
#define MAP_REMOVE(KEY_TYPE,MAP,KEY)\
	{ KEY_TYPE __key = KEY;map_remove(MAP,&__key);}
#endif

#ifndef MAP_FIND
#define MAP_FIND(KEY_TYPE,MAP,KEY)\
   ({  map_iter it;KEY_TYPE __key = KEY;\
       do it = map_find(MAP,&__key);\
       while(0);\
       it;})	
#endif

#endif