#include "datasocket.h"
#include "msg.h"

static void on_destroy(void *ptr)
{
	datasocket_t s = (datasocket_t)ptr;
	connection_destroy(&(s->c));
	free(ptr);
}

datasocket_t create_datasocket(struct engine_struct *e,struct connection *c,mq_t _mq)
{
	if(!c || !_mq)
		return NULL;
	datasocket_t s = (datasocket_t)calloc(1,sizeof(*s));
	s->c = c;
	s->_mq = _mq;
	s->is_close = 0;
	s->_refbase.mt = 1;
	s->_refbase.refcount = 1;
	s->_refbase.destroyer = on_destroy;
	c->custom_ptr = s;
	s->e = e;
	return s;
}

void close_datasocket(datasocket_t s)
{
	if(0 == s->is_close)
	{
		msg_t m = create_msg(s,MSG_ACTIVE_CLOSE);
		if(!m)
			return;
		mq_push(s->_mq,(list_node*)m);
		s->is_close = 1;
		release_datasocket(&s);
	}
}

void release_datasocket(datasocket_t *s)
{
	if(*s)
	{
		ref_decrease(&(*s)->_refbase);
		*s = 0;
	}
}

void acquire_datasocket(datasocket_t s)
{
	ref_increase(&s->_refbase);
} 

int32_t data_send(datasocket_t s,wpacket_t w)
{
	if(s->is_close)
		return -1;
	ref_increase(&s->_refbase);	
	w->ptr = s;
	mq_push(s->_mq,(list_node*)w);
	return 0;
}
