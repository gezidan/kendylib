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
	uint32_t len;     //����(ȥ���������ֶ�)
	uint32_t rpos;    //���±�
	uint32_t data_remain;
	uint32_t binbufpos;
	uint32_t begin_pos;
	buffer_t binbuf;       //���ڴ�ſ�Խbuffer_t�߽����ݵ�buffer_t
	buffer_t buf;          //��Ŵ����ݰ����ݵ�buffer_t����
	buffer_t readbuf;      //��ǰrpos���ڵ�buffer_t
	uint8_t  raw;          //ԭʼ�ֽ������ݰ�
	uint8_t  mt;
	uint64_t usr_data;
	allocator_t allocator;
}*rpacket_t;

struct wpacket;

rpacket_t rpacket_create(uint8_t mt,allocator_t _allo,buffer_t,uint32_t pos,uint32_t pk_len,uint8_t is_raw);
rpacket_t rpacket_create_by_wpacket(allocator_t _allo,struct wpacket*);//ͨ��wpacket����
rpacket_t rpacket_create_by_rpacket(rpacket_t);
void      rpacket_destroy(rpacket_t*);

//����һ����ԭʼ��(���߳�)
#define NEW_RPACKET(__BUF,__POS,__LEN,__ALLOCATOR) rpacket_create(0,__ALLOCATOR,__BUF,__POS,__LEN,0)
//����һ����ԭʼ��(���߳�)
#define NEW_RPACKET_MT(__BUF,__POS,__LEN,__ALLOCATOR) rpacket_create(1,__ALLOCATOR,__BUF,__POS,__LEN,0)

//����һ��ԭʼ��(���߳�)
#define NEW_RPACKET_RAW(__BUF,__POS,__LEN,__ALLOCATOR) rpacket_create(0,__ALLOCATOR,__BUF,__POS,__LEN,1)
//����һ����ԭʼ��(���߳�)
#define NEW_RPACKET_RAW_MT(__BUF,__POS,__LEN,__ALLOCATOR) rpacket_create(1,__ALLOCATOR,__BUF,__POS,__LEN,1)

//���ݶ�ȡ�ӿ�
static inline uint32_t  rpacket_len(rpacket_t r){
	return r->len;
}

static inline uint32_t rpacket_data_remain(rpacket_t r){
	return r->data_remain;
}

static inline int rpacket_read(rpacket_t r,int8_t *out,uint32_t size)
{
	if(r->data_remain < size)
		return -1;

	while(size>0)
	{
		uint32_t copy_size = r->readbuf->size - r->rpos;
		copy_size = copy_size >= size ? size:copy_size;
		memcpy(out,r->readbuf->buf + r->rpos,copy_size);
		size -= copy_size;
		r->rpos += copy_size;
		r->data_remain -= copy_size;
		out += copy_size;
		if(r->rpos >= r->readbuf->size && r->data_remain)
		{
			//��ǰbuffer�����Ѿ�������,�л�����һ��buffer
			r->rpos = 0;
			r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
		}
	}
	return 0;
}

static inline uint8_t rpacket_read_uint8(rpacket_t r)
{
	uint8_t value = 0;
	rpacket_read(r,(int8_t*)&value,sizeof(value));
	return value;
}

static inline uint16_t rpacket_read_uint16(rpacket_t r)
{
	uint16_t value = 0;
	rpacket_read(r,(int8_t*)&value,sizeof(value));
	return value;
}

static inline uint32_t rpacket_read_uint32(rpacket_t r)
{
	uint32_t value = 0;
	rpacket_read(r,(int8_t*)&value,sizeof(value));
	return value;
}

static inline uint64_t rpacket_read_uint64(rpacket_t r)
{
	uint64_t value = 0;
	rpacket_read(r,(int8_t*)&value,sizeof(value));
	return value;
}

static inline double rpacket_read_double(rpacket_t r)
{
	double value = 0;
	rpacket_read(r,(int8_t*)&value,sizeof(value));
	return value;
}

const char*    rpacket_read_string(rpacket_t);
const void*    rpacket_read_binary(rpacket_t,uint32_t *len);

#endif
