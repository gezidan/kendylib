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
#ifndef _WPACKET_H
#define _WPACKET_H
#include "buffer.h"
#include "link_list.h"
#include <stdint.h>
#include "allocator.h"
#include "sync.h"
#include <assert.h>

struct wpacket;
typedef void (*packet_send_finish)(void*,struct wpacket*);

typedef struct wpacket
{
	list_node next;
	uint8_t   type;
	uint32_t *len;      //包长字段(去除包长度字段以外实际数据的长度)在buf中的地址
	buffer_t buf;            //所有数据组成的buf链
	buffer_t writebuf;       //wpos所在的buf
	uint32_t wpos;
	uint8_t factor;
	uint32_t begin_pos; //属于本包的数据在首buf中的起始位置
	uint32_t data_size;//实际数据大小,包含包长度
	uint8_t  raw;
	uint8_t  mt;
	uint64_t usr_data;
	packet_send_finish _packet_send_finish;
	allocator_t allocator;
	uint32_t send_tick;//进入发送队列的时间
}*wpacket_t;
struct rpacket;


typedef struct
{
	buffer_t buf;
	uint32_t wpos;
}write_pos;

wpacket_t wpacket_create(uint8_t mt,allocator_t _allo,uint32_t size,uint8_t is_raw);
wpacket_t wpacket_create_by_rpacket(allocator_t _allo,struct rpacket*);//通过rpacket构造
void wpacket_destroy(wpacket_t*);

//创建一个非原始包(单线程)
#define NEW_WPACKET(__SIZE,__ALLOCATOR) wpacket_create(0,__ALLOCATOR,__SIZE,0)
//创建一个非原始包(多线程)
#define NEW_WPACKET_MT(__SIZE,__ALLOCATOR) wpacket_create(1,__ALLOCATOR,__SIZE,0)

//创建一个原始包(单线程)
#define NEW_WPACKET_RAW(__SIZE,__ALLOCATOR) wpacket_create(0,__ALLOCATOR,__SIZE,1)
//创建一个非原始包(多线程)
#define NEW_WPACKET_RAW_MT(__SIZE,__ALLOCATOR) wpacket_create(1,__ALLOCATOR,__SIZE,1)

static inline write_pos wpacket_get_writepos(wpacket_t w)
{
	write_pos wp = {w->writebuf,w->wpos};
	return wp;
}

static inline void wpacket_rewrite(write_pos *wp,int8_t *addr,uint32_t size)
{
	int8_t *ptr = addr;
	uint32_t copy_size;
	uint32_t pos = wp->wpos;
	while(size)
	{
		copy_size = wp->buf->capacity - pos;
		copy_size = copy_size > size ? size:copy_size;
		memcpy(wp->buf->buf + pos,ptr,copy_size);
		ptr += copy_size;
		size -= copy_size;
		pos += copy_size;
		if(size && pos >= wp->buf->capacity)
		{
			assert(wp->buf->next);
			wp->buf = wp->buf->next;
			pos = 0;
		}

	}
}

static inline void wpacket_rewrite_uint8(write_pos *wp,uint8_t value)
{
	wpacket_rewrite(wp,(int8_t*)&value,sizeof(value));
}

static inline void wpacket_rewrite_uint16(write_pos *wp,uint16_t value)
{
	wpacket_rewrite(wp,(int8_t*)&value,sizeof(value));
}

static inline void wpacket_rewrite_uint32(write_pos *wp,uint32_t value)
{
	wpacket_rewrite(wp,(int8_t*)&value,sizeof(value));
}

static inline void wpacket_rewrite_uint64(write_pos *wp,uint64_t value)
{
	wpacket_rewrite(wp,(int8_t*)&value,sizeof(value));
}

static inline void wpacket_rewrite_double(write_pos *wp,double value)
{
	wpacket_rewrite(wp,(int8_t*)&value,sizeof(value));
}

static inline void wpacket_expand(wpacket_t w)
{
	uint32_t size;
	w->factor <<= 1;
	size = w->factor;
	w->writebuf->next = buffer_create_and_acquire(w->mt,NULL,size);
	w->writebuf = buffer_acquire(w->writebuf,w->writebuf->next); 
	w->wpos = 0;
}


static inline void wpacket_copy(wpacket_t w,buffer_t buf)
{
	int8_t *ptr = buf->buf;
	buffer_t tmp_buf = w->buf;
	uint32_t copy_size;
	while(tmp_buf)
	{
		copy_size = tmp_buf->size - w->wpos;
		memcpy(ptr,tmp_buf->buf,copy_size);
		ptr += copy_size;
		w->wpos = 0;
		tmp_buf = tmp_buf->next;
	}
}

static inline void do_write_copy(wpacket_t w)
{
	/*wpacket是由rpacket构造的，这里执行写时拷贝，
	* 执行完后wpacket和构造时传入的rpacket不再共享buffer
	*/
	w->factor = GetSize_of_pow2(*w->len);
	buffer_t tmp = buffer_create_and_acquire(w->mt,NULL,w->factor);
	wpacket_copy(w,tmp);
	w->begin_pos = 0;
	if(!w->raw)
	{
		w->len = (uint32_t*)tmp->buf;
		w->wpos = sizeof(*w->len);
	}
	w->buf = buffer_acquire(w->buf,tmp);
	w->writebuf = buffer_acquire(w->writebuf,w->buf);
}

static inline void wpacket_write(wpacket_t w,int8_t *addr,uint32_t size)
{
	int8_t *ptr = addr;
	uint32_t copy_size;
	if(!w->writebuf){
		do_write_copy(w);
	}
	while(size)
	{
		copy_size = w->writebuf->capacity - w->wpos;
		if(copy_size == 0)
		{
			wpacket_expand(w);//空间不足,扩展
			copy_size = w->writebuf->capacity - w->wpos;
		}
		copy_size = copy_size > size ? size:copy_size;
		memcpy(w->writebuf->buf + w->wpos,ptr,copy_size);
		w->writebuf->size += copy_size;
		if(w->len)
			(*w->len) += copy_size;
		w->wpos += copy_size;
		ptr += copy_size;
		size -= copy_size;
		w->data_size += copy_size;
	}
}


static inline void wpacket_write_uint8(wpacket_t w,uint8_t value)
{
	wpacket_write(w,(int8_t*)&value,sizeof(value));
}

static inline void wpacket_write_uint16(wpacket_t w,uint16_t value)
{
	wpacket_write(w,(int8_t*)&value,sizeof(value));
}

static inline void wpacket_write_uint32(wpacket_t w,uint32_t value)
{
	wpacket_write(w,(int8_t*)&value,sizeof(value));
}

static inline void wpacket_write_uint64(wpacket_t w,uint64_t value)
{
	wpacket_write(w,(int8_t*)&value,sizeof(value));
}

static inline void wpacket_write_double(wpacket_t w ,double value)
{
	wpacket_write(w,(int8_t*)&value,sizeof(value));
}

static inline void wpacket_write_binary(wpacket_t w,const void *value,uint32_t size)
{
	assert(value);
	if(!w->raw)
		wpacket_write_uint32(w,size);
	wpacket_write(w,(int8_t*)value,size);
}

static inline void wpacket_write_string(wpacket_t w ,const char *value)
{
	if(w->raw)
		wpacket_write_binary(w,value,strlen(value));
	else
		wpacket_write_binary(w,value,strlen(value)+1);
}

#endif
