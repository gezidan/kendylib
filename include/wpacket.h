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

write_pos wpacket_get_writepos(wpacket_t);
void wpacket_write_uint8(wpacket_t,uint8_t);
void wpacket_write_uint16(wpacket_t,uint16_t);
void wpacket_write_uint32(wpacket_t,uint32_t);
void wpacket_write_uint64(wpacket_t,uint64_t);
void wpacket_write_double(wpacket_t,double);

void wpacket_rewrite_uint8(write_pos*,uint8_t);
void wpacket_rewrite_uint16(write_pos*,uint16_t);
void wpacket_rewrite_uint32(write_pos*,uint32_t);
void wpacket_rewrite_uint64(write_pos*,uint64_t);
void wpacket_rewrite_double(write_pos*,double);

//不提供对非定长数据的rewrite
void wpacket_write_string(wpacket_t,const char*);
void wpacket_write_binary(wpacket_t,const void*,uint32_t);

#endif
