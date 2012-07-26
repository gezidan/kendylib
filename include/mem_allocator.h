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
#ifndef _MEM_ALLOCATOR_H
#define _MEM_ALLOCATOR_H
#include <stdint.h>
#include "../include/allocator.h"

typedef struct general_allocator *general_allocator_t;
/*
* max:可以分配的最大请求字节数
*/
struct allocator* gen_allocator_create(uint32_t max);
void   gen_allocator_destroy(struct allocator**);

void  *gen_allocator_alloc(struct allocator*,int32_t);
void   gen_allocator_dealloc(struct allocator*,void*);

//void show_fix_info(struct allocator);
//void show_first_fit_info(struct allocator);

#endif
