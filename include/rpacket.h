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
#ifndef _RPACKET_H
#define _RPACKET_H

#include "buffer.h"
#include "link_list.h"
#include <stdint.h>
#include "allocator.h"
#include "sync.h"
typedef struct rpacket
{
	list_node next;
	uint8_t  type;
	uint32_t len;     //包长(去除包长度字段)
	uint32_t rpos;    //读下标
	uint32_t data_remain;
	uint32_t binbufpos;
	uint32_t begin_pos;
	buffer_t binbuf;       //用于存放跨越buffer_t边界数据的buffer_t
	buffer_t buf;          //存放此数据包内容的buffer_t链表
	buffer_t readbuf;      //当前rpos所在的buffer_t
	uint8_t  raw;          //原始字节流数据包
	uint8_t  mt;
	uint64_t usr_data;
	allocator_t allocator;
}*rpacket_t;

struct wpacket;

rpacket_t rpacket_create(uint8_t mt,allocator_t _allo,buffer_t,uint32_t pos,uint32_t pk_len,uint8_t is_raw);
rpacket_t rpacket_create_by_wpacket(allocator_t _allo,struct wpacket*);//通过wpacket构造
rpacket_t rpacket_create_by_rpacket(rpacket_t);
void      rpacket_destroy(rpacket_t*);

//数据读取接口
uint32_t  rpacket_len(rpacket_t);
uint32_t  rpacket_data_remain(rpacket_t);
uint8_t rpacket_read_uint8(rpacket_t);
uint16_t rpacket_read_uint16(rpacket_t);
uint32_t rpacket_read_uint32(rpacket_t);
uint64_t rpacket_read_uint64(rpacket_t);

double         rpacket_read_double(rpacket_t);
const char*    rpacket_read_string(rpacket_t);
const void*    rpacket_read_binary(rpacket_t,uint32_t *len);

#endif
