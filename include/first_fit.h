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
#ifndef _FIRST_FIT_H
#define _FIRST_FIT_H

#include <stdint.h>

/*
* 首次适应算法,添加tag,避免Free时的搜索,TAOCP动态内存分配算法C
*/


typedef struct first_fit_pool *first_fit_t;

#include "../include/allocator.h"


extern struct allocator *first_fit_create(uint32_t size);

extern void first_fit_destroy(struct allocator **pool);

extern void *first_fit_alloc(struct allocator*,int32_t size);

extern void first_fit_dealloc(struct allocator*,void *ptr);

//extern uint32_t first_fit_get_free_size(first_fit_t);

#endif
