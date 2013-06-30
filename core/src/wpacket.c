#include "wpacket.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "rpacket.h"
#include "common_define.h"
#include "atomic.h"

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


void wpacket_destroy(wpacket_t *w)
{
	buffer_release(&(*w)->buf);
	buffer_release(&(*w)->writebuf);
	FREE((*w)->allocator,*w);
	*w = NULL;
}

