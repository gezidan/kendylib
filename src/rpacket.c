#include "rpacket.h"
#include "wpacket.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common_define.h"
#include "atomic.h"

rpacket_t rpacket_create(uint8_t mt,allocator_t _alloc,buffer_t b,uint32_t pos,uint32_t pk_len,uint8_t is_raw)
{
	rpacket_t r = (rpacket_t)ALLOC(_alloc,sizeof(*r));	
	r->allocator = _alloc;
	r->mt = mt;
	r->binbuf = NULL;
	r->binbufpos = 0;
	r->buf = buffer_acquire(NULL,b);
	r->readbuf = buffer_acquire(NULL,b);
	r->len = pk_len;
	r->data_remain = r->len;
	r->begin_pos = pos;
	r->next.next = NULL;
	r->type = MSG_RPACKET;
	
	if(is_raw)
		r->rpos = pos;
	else
		r->rpos = (pos + sizeof(r->len))%r->buf->capacity;
	if(r->rpos < r->begin_pos)
		r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
		
	r->raw = is_raw;
	return r;
}

rpacket_t rpacket_create_by_rpacket(rpacket_t other)
{
	rpacket_t r = (rpacket_t)ALLOC(other->allocator,sizeof(*r));
	r->allocator = other->allocator;
	r->mt = other->mt;
	r->binbuf = NULL;
	r->binbufpos = 0;
	r->buf = buffer_acquire(NULL,other->buf);
	r->readbuf = buffer_acquire(NULL,other->readbuf);
	r->len = other->len;
	r->data_remain = other->len;
	r->begin_pos = other->begin_pos;
	r->next.next = NULL;
	r->type = MSG_RPACKET;
	r->raw = other->raw;
	r->rpos = other->rpos;	
	return r;	
}

rpacket_t rpacket_create_by_wpacket(allocator_t _alloc,struct wpacket *w)
{
	rpacket_t r = (rpacket_t)ALLOC(_alloc,sizeof(*r));	
	r->allocator = _alloc;
	r->binbuf = NULL;
	r->mt = w->mt;
	r->binbufpos = 0;
	r->buf = buffer_acquire(NULL,w->buf);
	r->readbuf = buffer_acquire(NULL,w->buf);
	r->raw = w->raw;
	r->begin_pos = w->begin_pos;
	r->next.next = NULL;
	r->type = MSG_RPACKET;	
	if(r->raw)
	{
		r->len = w->data_size;
		r->rpos =w->begin_pos;
	}
	else
	{
		//这里的len只记录构造时wpacket的len,之后wpacket的写入不会影响到rpacket的len
		r->len = w->data_size - sizeof(r->len);
		r->rpos = (r->begin_pos + sizeof(r->len))%r->buf->capacity;
		if(r->rpos < r->begin_pos)
			r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
	}
	r->data_remain = r->len;
	return r;
}

void      rpacket_destroy(rpacket_t *r)
{
	//释放所有对buffer_t的引用
	buffer_release(&(*r)->buf);
	buffer_release(&(*r)->readbuf);
	buffer_release(&(*r)->binbuf);
	FREE((*r)->allocator,*r);	
	*r = 0;
}

uint32_t  rpacket_len(rpacket_t r)
{
	return r->len;
}

uint32_t  rpacket_data_remain(rpacket_t r)
{
	return r->data_remain;
}

static int rpacket_read(rpacket_t r,int8_t *out,uint32_t size)
{
	buffer_t _next = 0;
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
			//当前buffer数据已经被读完,切换到下一个buffer
			r->rpos = 0;
			r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
		}
	}
	return 0;
}

uint8_t rpacket_read_uint8(rpacket_t r)
{
	uint8_t value = 0;
	rpacket_read(r,(int8_t*)&value,sizeof(value));
	return value;
}

uint16_t rpacket_read_uint16(rpacket_t r)
{
	uint16_t value = 0;
	rpacket_read(r,(int8_t*)&value,sizeof(value));
	return value;
}

uint32_t rpacket_read_uint32(rpacket_t r)
{
	uint32_t value = 0;
	rpacket_read(r,(int8_t*)&value,sizeof(value));
	return value;
}

uint64_t rpacket_read_uint64(rpacket_t r)
{
	uint64_t value = 0;
	rpacket_read(r,(int8_t*)&value,sizeof(value));
	return value;
}

double   rpacket_read_double(rpacket_t r)
{
	double value = 0;
	rpacket_read(r,(int8_t*)&value,sizeof(value));
	return value;
}



static const void* rpacket_raw_read_binary(rpacket_t r,uint32_t *len)
{
	void *addr = 0;
	uint32_t size = 0;
	if(!r->data_remain)
		return addr;
	if(r->readbuf->size - r->rpos >= r->data_remain)
	{
		*len = r->data_remain;
		r->data_remain = 0;
		addr = &r->readbuf->buf[r->rpos];
		r->rpos += r->data_remain;
	}
	else
	{
		if(!r->binbuf)
		{
			r->binbufpos = 0;
			r->binbuf = buffer_create_and_acquire(r->mt,NULL,r->len);
		}
		addr = r->binbuf->buf + r->binbufpos;
		size = r->data_remain;
		while(size)
		{
			uint32_t copy_size = r->readbuf->size - r->rpos;
			copy_size = copy_size >= size ? size:copy_size;
			memcpy(r->binbuf->buf + r->binbufpos,r->readbuf->buf + r->rpos,copy_size);
			size -= copy_size;
			r->rpos += copy_size;
			r->data_remain -= copy_size;
			r->binbufpos += copy_size;		
			if(r->rpos >= r->readbuf->size && r->data_remain)
			{
				//当前buffer数据已经被读完,切换到下一个buffer
				r->rpos = 0;
				r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
			}
		}
	}
	return addr;
}

const char* rpacket_read_string(rpacket_t r)
{
	uint32_t len = 0;
	if(r->raw)//raw类型的rpacket不支持读取字符串
		return rpacket_raw_read_binary(r,&len);
	return (const char *)rpacket_read_binary(r,&len);
}

const void* rpacket_read_binary(rpacket_t r,uint32_t *len)
{
	void *addr = 0;
	uint32_t size = 0;
	if(r->raw)
		return rpacket_raw_read_binary(r,len);
	size = rpacket_read_uint32(r);
	*len = size;
	if(!r->data_remain || r->data_remain < size)
		return addr;
	if(r->readbuf->size - r->rpos >= size)
	{
		addr = &r->readbuf->buf[r->rpos];
		r->rpos += size;
		r->data_remain -= size;
		if(r->rpos >= r->readbuf->size && r->data_remain)
		{
			//当前buffer数据已经被读完,切换到下一个buffer
			r->rpos = 0;
			r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
		}
	}
	else
	{
		//数据跨越了buffer边界,创建binbuf,将数据拷贝到binbuf中
		if(!r->binbuf)
		{
			r->binbufpos = 0;
			r->binbuf = buffer_create_and_acquire(r->mt,NULL,r->len);
		}
		addr = r->binbuf->buf + r->binbufpos;
		while(size)
		{
			uint32_t copy_size = r->readbuf->size - r->rpos;
			copy_size = copy_size >= size ? size:copy_size;
			memcpy(r->binbuf->buf + r->binbufpos,r->readbuf->buf + r->rpos,copy_size);
			size -= copy_size;
			r->rpos += copy_size;
			r->data_remain -= copy_size;
			r->binbufpos += copy_size;		
			if(r->rpos >= r->readbuf->size && r->data_remain)
			{
				//当前buffer数据已经被读完,切换到下一个buffer
				r->rpos = 0;
				r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
			}
		}

	}
	return addr;
}
