#include "rpacket.h"
#include "wpacket.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common_define.h"
#include "atomic.h"

rpacket_t rpacket_create(uint8_t mt,
						 allocator_t _alloc,
						 buffer_t b,
						 uint32_t pos,
						 uint32_t pk_len,
						 uint8_t is_raw)
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
		//�����lenֻ��¼����ʱwpacket��len,֮��wpacket��д�벻��Ӱ�쵽rpacket��len
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
	//�ͷ����ж�buffer_t������
	buffer_release(&(*r)->buf);
	buffer_release(&(*r)->readbuf);
	buffer_release(&(*r)->binbuf);
	FREE((*r)->allocator,*r);	
	*r = 0;
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
				//��ǰbuffer�����Ѿ�������,�л�����һ��buffer
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
	if(r->raw)//raw���͵�rpacket��֧�ֶ�ȡ�ַ���
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
			//��ǰbuffer�����Ѿ�������,�л�����һ��buffer
			r->rpos = 0;
			r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
		}
	}
	else
	{
		//���ݿ�Խ��buffer�߽�,����binbuf,�����ݿ�����binbuf��
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
				//��ǰbuffer�����Ѿ�������,�л�����һ��buffer
				r->rpos = 0;
				r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
			}
		}

	}
	return addr;
}
