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
/*
 * discard
*/
 	
#ifndef _FIX_OBJ_POOL_H
#define _FIX_OBJ_POOL_H

#include <stdint.h>
typedef struct fix_obj_pool *fix_pool_t;

#include "../include/allocator.h"

/*
* obj_size:对象大小
* default_size:默认对象池大小
* align4:返回的对象地址是否需要对齐到4字节
*/
extern struct allocator* create_pool(uint32_t obj_size,int32_t default_size,int32_t align4);

extern void destroy_pool(struct allocator **pool);

/*
* 分配和回收
*/
extern void *pool_alloc(struct allocator*,int32_t);

extern void  pool_dealloc(struct allocator*,void*);

extern uint32_t alignsize(uint32_t obj_size);

//extern uint32_t get_free_size(fix_pool_t);

#endif
