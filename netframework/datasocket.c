#include "datasocket.h"
#include "msg.h"

static void on_destroy(void *ptr)
{
	datasocket_t s = (datasocket_t)ptr;
	HANDLE sock = s->c->socket;
	connection_destroy(&(s->c));
	ReleaseSocketWrapper(sock);
	free(ptr);
	printf("destroy datasocket\n");
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
	{
		wpacket_destroy(&w);
		return -1;
	}
	ref_increase(&s->_refbase);	
	w->ptr = s;
	mq_push(s->_mq,(list_node*)w);
	return 0;
}

int32_t set_recv_timeout(datasocket_t s,uint32_t ms)
{
	if(ms > MAX_WHEEL_TIME)
		ms = MAX_WHEEL_TIME;
	if(s->c->recv_timeout > 0)
		return -1;
	s->c->recv_timeout = ms;
	if(s->c->send_timeout > 0)
		return 0;
	msg_t m = create_msg(s,MSG_SET_RECV_TIMEOUT);
	if(!m)
		return -1;
	ref_increase(&s->_refbase);	
	mq_push(s->_mq,(list_node*)m);
	mq_flush();
	return 0;	
}

int32_t set_send_timeout(datasocket_t s,uint32_t ms)
{
	if(ms > MAX_WHEEL_TIME)
		ms = MAX_WHEEL_TIME;	
	if(s->c->send_timeout > 0)
		return -1;
	s->c->send_timeout = ms;
	if(s->c->recv_timeout > 0)
		return 0;
	msg_t m = create_msg(s,MSG_SET_SEND_TIMEOUT);
	if(!m)
		return -1;
	ref_increase(&s->_refbase);		
	mq_push(s->_mq,(list_node*)m);
	mq_flush();
	return 0;
}
