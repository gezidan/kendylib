#include "wpacket.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "rpacket.h"
#include "common_define.h"
#include "atomic.h"


inline int32_t is_pow_of_2(uint32_t size)
{
	return !(size&(size-1));
}

inline uint8_t GetK(uint32_t size)
{
	uint8_t k = 0;
	if(!is_pow_of_2(size))
	{
		size = (size << 1);
	}
	while(size > 1)
	{
		k++;
		size = size >> 1;
	}
	return k;
}

inline uint32_t GetSize_of_pow2(uint32_t size)
{
	if(is_pow_of_2(size))
		return size;
	size = size-1;
	size = size | (size>>1);
	size = size | (size>>2);
	size = size | (size>>4);
	size = size | (size>>8);
	size = size | (size>>16);
	return size + 1;
	//return 	1 << GetK(size);
}

wpacket_t wpacket_create(uint8_t mt,allocator_t _allo,uint32_t size,uint8_t is_raw)
{
	size = GetSize_of_pow2(size);
	wpacket_t w = (wpacket_t)ALLOC(_allo,sizeof(*w));	
	w->allocator = _allo;
	w->mt = mt;	
	w->factor = size;
	w->raw = is_raw;
	w->buf = buffer_create_and_acquire(mt,NULL,size);
	w->writebuf = buffer_acquire(NULL,w->buf);
	w->begin_pos = 0;
	w->next.next = NULL;
	w->_packet_send_finish = NULL;
	w->type = MSG_WPACKET;
	if(is_raw)
	{
		w->wpos = 0;
		w->len = 0;
		w->buf->size = 0;
		w->data_size = 0;
	}
	else
	{
		w->wpos = sizeof(*(w->len));
		w->len = (uint32_t*)w->buf->buf;
		*(w->len) = 0;
		w->buf->size = sizeof(*(w->len));
		w->data_size = sizeof(*(w->len));
	}
	//ATOMIC_INCREASE(&wpacket_count);
	return w;
}

wpacket_t wpacket_create_by_rpacket(allocator_t _allo,struct rpacket *r)
{
	wpacket_t w = (wpacket_t)ALLOC(_allo,sizeof(*w));	
	w->allocator = _allo;	
	w->raw = r->raw;
	w->mt = r->mt;
	w->factor = 0;
	w->writebuf = NULL;
	w->begin_pos = r->begin_pos;
	w->buf = buffer_acquire(NULL,r->buf);
	w->len = 0;//触发拷贝之前len没有作用
	w->wpos = 0;
	w->next.next = NULL;
	w->_packet_send_finish = NULL;
	w->type = MSG_WPACKET;
	if(w->raw)
		w->data_size = r->len;
	else
		w->data_size = r->len + sizeof(r->len);
	return w;
}

write_pos wpacket_get_writepos(wpacket_t w)
{
	write_pos wp = {w->writebuf,w->wpos};
	return wp;
}

void wpacket_destroy(wpacket_t *w)
{
	buffer_release(&(*w)->buf);
	buffer_release(&(*w)->writebuf);
	FREE((*w)->allocator,*w);
	*w = NULL;
	//ATOMIC_DECREASE(&wpacket_count);
}

static void wpacket_expand(wpacket_t w)
{
	uint32_t size;
	w->factor <<= 1;
	size = w->factor;
	w->writebuf->next = buffer_create_and_acquire(w->mt,NULL,size);
	w->writebuf = buffer_acquire(w->writebuf,w->writebuf->next); 
	w->wpos = 0;
}


static void wpacket_copy(wpacket_t w,buffer_t buf)
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

static void wpacket_write(wpacket_t w,int8_t *addr,uint32_t size)
{
	int8_t *ptr = addr;
	uint32_t copy_size;
	buffer_t tmp;
	uint8_t k;
	if(!w->writebuf)
	{
		/*wpacket是由rpacket构造的，这里执行写时拷贝，
		* 执行完后wpacket和构造时传入的rpacket不再共享buffer
		*/
		w->factor = GetSize_of_pow2(*w->len);
		tmp = buffer_create_and_acquire(w->mt,NULL,w->factor);
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


void wpacket_write_uint8(wpacket_t w,uint8_t value)
{
	wpacket_write(w,(uint8_t*)&value,sizeof(value));
}

void wpacket_write_uint16(wpacket_t w,uint16_t value)
{
	wpacket_write(w,(uint8_t*)&value,sizeof(value));
}

void wpacket_write_uint32(wpacket_t w,uint32_t value)
{
	wpacket_write(w,(uint8_t*)&value,sizeof(value));
}

void wpacket_write_uint64(wpacket_t w,uint64_t value)
{
	wpacket_write(w,(uint8_t*)&value,sizeof(value));
}

void wpacket_write_double(wpacket_t w ,double value)
{
	wpacket_write(w,(uint8_t*)&value,sizeof(value));
}

static void wpacket_rewrite(write_pos *wp,int8_t *addr,uint32_t size)
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

void wpacket_rewrite_uint8(write_pos *wp,uint8_t value)
{
	wpacket_rewrite(wp,(uint8_t*)&value,sizeof(value));
}

void wpacket_rewrite_uint16(write_pos *wp,uint16_t value)
{
	wpacket_rewrite(wp,(uint8_t*)&value,sizeof(value));
}

void wpacket_rewrite_uint32(write_pos *wp,uint32_t value)
{
	wpacket_rewrite(wp,(uint8_t*)&value,sizeof(value));
}

void wpacket_rewrite_uint64(write_pos *wp,uint64_t value)
{
	wpacket_rewrite(wp,(uint8_t*)&value,sizeof(value));
}

void wpacket_rewrite_double(write_pos *wp,double value)
{
	wpacket_rewrite(wp,(uint8_t*)&value,sizeof(value));
}

void wpacket_write_string(wpacket_t w ,const char *value)
{
	if(w->raw)
		wpacket_write_binary(w,value,strlen(value));
	else
		wpacket_write_binary(w,value,strlen(value)+1);
}

void wpacket_write_binary(wpacket_t w,const void *value,uint32_t size)
{
	assert(value);
	if(!w->raw)
		wpacket_write_uint32(w,size);
	wpacket_write(w,(int8_t*)value,size);
}
