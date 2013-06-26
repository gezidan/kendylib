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
#ifndef _LOCAL_POOL_H
#define _LOCAL_POOL_H
#include <stdint.h>
typedef struct local_pool *local_pool_t;
#include "allocator.h"


extern struct allocator* local_pool_create(void*,int32_t size);
extern void   local_pool_destroy(struct allocator **);

extern void *local_pool_alloc(struct allocator*,int32_t);
extern void  local_pool_dealloc(struct allocator*,void*);


#endif
