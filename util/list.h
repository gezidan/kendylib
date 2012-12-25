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
#ifndef _LIST_H
#define _LIST_H
#include <stdint.h>
typedef struct list *list_t;
struct node;

typedef struct list_iter
{
	struct node **next;
	struct node *n;
}list_iter;

extern list_t list_create(uint32_t val_size);
extern void   list_destroy(list_t*);

extern list_iter list_begin(list_t);
extern list_iter list_end(list_t);
extern list_iter list_rbegin(list_t);
extern list_iter list_rend(list_t);

extern uint32_t list_size(list_t);
extern void   list_insert_before(list_t,list_iter,void*);
extern void   list_insert_after(list_t,list_iter,void*);
extern void   list_push_back(list_t,void*);
extern void   list_push_front(list_t,void*);
extern void   list_pop_back(list_t,void*);
extern void   list_pop_front(list_t,void*);
extern void   list_front(list_t,void*);
extern void   list_back(list_t,void*);

extern int32_t    list_is_empty(list_t);

#ifndef LIST_CREATE
#define LIST_CREATE(TYPE)\
	list_create(sizeof(TYPE))
#endif

#ifndef LIST_INSERT_BEFORE
#define LIST_INSERT_BEFORE(TYPE,LIST,IT,VAL)\
	{TYPE __val = VAL;list_insert_before(LIST,IT,&__val);}
#endif

#ifndef LIST_INSERT_AFTER
#define LIST_INSERT_AFTER(TYPE,LIST,IT,VAL)\
	{TYPE __val = VAL;list_insert_after(LIST,IT,&__val);}
#endif

#ifndef LIST_PUSH_BACK
#define LIST_PUSH_BACK(TYPE,LIST,VAL)\
	{TYPE __val = VAL;list_push_back(LIST,&__val);}
#endif

#ifndef LIST_PUSH_FRONT
#define LIST_PUSH_FRONT(TYPE,LIST,VAL)\
	{TYPE __val = VAL;list_push_front(LIST,&__val);}
#endif

#ifndef LIST_POP_FRONT
#define LIST_POP_FRONT(TYPE,LIST)\
   ({ TYPE __result;\
       do list_pop_front(LIST,&__result);\
       while(0);\
       __result;})
#endif

#ifndef LIST_FRONT
#define LIST_FRONT(TYPE,LIST)\
	({ TYPE __result;\
	do list_front(LIST,&__result);\
	while(0);\
	__result;})
#endif

#ifndef LIST_POP_BACK
#define LIST_POP_BACK(TYPE,LIST)\
   ({ TYPE __result;\
       do list_pop_back(LIST,&__result);\
       while(0);\
       __result;})
#endif

#ifndef LIST_BACK
#define LIST_BACK(TYPE,LIST)\
	({ TYPE __result;\
	do list_back(LIST,&__result);\
	while(0);\
	__result;})
#endif




extern list_iter list_find(list_t,void*);

#ifndef LIST_FIND
#define LIST_FIND(TYPE,L,VAL)\
   ({ TYPE __val = VAL;list_iter it;\
       do it = list_find(L,&__val);\
       while(0);\
       it;})
#endif

extern int32_t    list_remove(list_t,void*);

#ifndef LIST_REMOVE
#define LIST_REMOVE(TYPE,L,VAL)\
   ({ TYPE __val = VAL;int32_t __ret;\
       do __ret = list_remove(L,&__val);\
       while(0);\
       __ret;})
#endif

extern list_iter list_erase(list_t,list_iter);

extern list_iter list_iter_next(list_iter);
extern void  list_iter_get_val(list_iter,void*);
extern void  list_iter_set_val(list_iter,void*);
extern int32_t   list_iter_is_equal(list_iter a,list_iter b);


#ifndef IT_LIST_NEXT
#define IT_LIST_NEXT(ITER)\
	list_iter_next(ITER)
#endif

#ifndef IT_LIST_EQUAL
#define IT_LIST_EQUAL(IT1,IT2)\
	list_iter_is_equal(IT1,IT2)
#endif

#ifndef IT_LIST_GET
#define IT_LIST_GET(TYPE,NODE)\
   ({ TYPE __result;\
       do list_iter_get_val(NODE,&__result);\
       while(0);\
       __result;})
#endif

#ifndef IT_LIST_SET
#define IT_LIST_SET(TYPE,NODE,VAL)\
	{TYPE __val=VAL;iter_set_val(NODE,&__val);}
#endif

#endif
