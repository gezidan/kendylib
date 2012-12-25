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
//内存分配器接口类
#ifndef _ALLOCATOR_H
#define _ALLOCATOR_H
#include <stdint.h>
//内存分配器接口类
typedef struct allocator
{
	void* (*Alloc)(struct allocator*,int32_t);
	void (*DeAlloc)(struct allocator*,void*);
	void (*Destroy)(struct allocator**);
}*allocator_t;

#ifndef IMPLEMEMT
#define IMPLEMEMT(SUPER_CLASS) struct SUPER_CLASS super_class
#endif

#ifndef ALLOC
#define ALLOC(ALLOCATOR,SIZE)\
   ({ void *__result;\
       do \
	   if(ALLOCATOR)\
	     __result = ((struct allocator*)ALLOCATOR)->Alloc(ALLOCATOR,SIZE);\
	   else\
	     __result = calloc(1,SIZE);\
	   while(0);\
       __result;})
#endif

#ifndef FREE	
#define FREE(ALLOCATOR,PTR)\
   ({\
       do \
	   if(ALLOCATOR)\
	     ((struct allocator*)ALLOCATOR)->DeAlloc(ALLOCATOR,PTR);\
	   else\
	     free(PTR);\
	   while(0);\
		})
#endif

#ifndef DESTROY
#define DESTROY(ALLOCATOR)\
	((struct allocator*)(*(ALLOCATOR)))->Destroy((ALLOCATOR))
#endif
	
#endif
