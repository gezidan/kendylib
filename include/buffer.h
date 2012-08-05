#ifndef _BUFFER_H
#define _BUFFER_H
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
* 带引用计数的buffer
*/

#include "KendyNet.h"
#include <stdint.h>
#include "refbase.h"
#include "sync.h"
#include "link_list.h"
#include "spinlock.h"
typedef struct buffer
{
	struct refbase _refbase;
	uint32_t capacity;
	uint32_t size;
	struct buffer *next;
	int8_t   buf[0];
}*buffer_t;


inline buffer_t buffer_create_and_acquire(uint8_t mt,buffer_t,uint32_t);
inline buffer_t buffer_acquire(buffer_t,buffer_t);
inline void     buffer_release(buffer_t*);
int32_t  buffer_read(buffer_t,uint32_t,int8_t*,uint32_t);

/*
struct _mem_block
{
	list_node next;
	void *mem_block;
};

struct buffer_block
{
	list_node next;
};

struct buffer_mgr
{
	int32_t factor;
	int32_t free_size;
	list_node *head;
	list_node *tail;
	struct link_list *blocks;
	int32_t create_block_size;
	//mutex_t mtx;
	spinlock_t mtx;
};

typedef struct buffer_allocator
{
	IMPLEMEMT(allocator);
	struct buffer_mgr bf_mgr[11];//2^5~2^16
}*buffer_allocator_t;


buffer_allocator_t create_buffer_allocator(int8_t mt);
*/

#endif
