#include "msg_loop.h"
#include "SysTime.h"

msg_loop_t create_msg_loop()
{
	msg_loop_t m = (msg_loop_t)calloc(1,sizeof(*m));
	m->last_sync_tick = GetCurrentMs();
	return m;
}

void msg_loop_register_callback(msg_loop_t m ,uint16_t type,msg_callback callback)
{
	if(type < MSG_END)
		m->callbacks[type] = callback;
}


static inline void dispatch_msg(msg_loop_t m,msg_t _msg)
{
	switch(_msg->type)
	{
		case MSG_RPACKET:
			{
				rpacket_t r = (rpacket_t)_msg;
				m->callbacks[MSG_RPACKET]((datasocket_t)r->ptr,(void*)r);
				rpacket_destroy(&r);
			}
			break;
		case MSG_NEW_CONNECTION:
			{
				m->callbacks[MSG_RPACKET]((datasocket_t)_msg->ptr,NULL);
				destroy_msg(&_msg);
			}
			break;
		case MSG_DISCONNECTED:
			{
				datasocket_t s = (datasocket_t)_msg->ptr;
				m->callbacks[MSG_RPACKET](s,(void*)s->close_reason);
				destroy_msg(&_msg);
			}
			break;
		case MSG_SEND_BLOCK:
			{
				datasocket_t s = (datasocket_t)_msg->ptr;
				m->callbacks[MSG_RPACKET](s,NULL);
				destroy_msg(&_msg);			
			}
			break;
	}
}

void msg_loop_once(msg_loop_t m,netservice_t s,uint32_t ms)
{
	msg_t _msg = NULL;
	uint32_t timeout = GetCurrentMs() + ms;
	uint32_t now_tick;
	do
	{
		_msg = net_peek_msg(s,5);
		if(_msg)
			dispatch_msg(m,_msg);
		now_tick = GetCurrentMs();
		if(now_tick - m->last_sync_tick >= 10)
		{
			m->last_sync_tick = now_tick;
			mq_flush();
		}		
	}while(now_tick < timeout);
}

void destroy_msg_loop(msg_loop_t *m)
{
	free(*m);
	*m = NULL;
}
