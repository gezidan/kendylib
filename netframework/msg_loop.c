#include "msg_loop.h"
#include "SysTime.h"

msg_loop_t create_msg_loop(on_packet _on_packet,on_new_connection _on_new_connection,on_connection_disconnect _on_connection_disconnect)
{
	msg_loop_t m = (msg_loop_t)calloc(1,sizeof(*m));
	m->_on_packet = _on_packet;
	m->_on_new_connection = _on_new_connection;
	m->_on_connection_disconnect = _on_connection_disconnect;
	return m;
}


static inline void dispatch_msg(msg_loop_t m,msg_t _msg)
{
	switch(_msg->type)
	{
		case MSG_RPACKET:
			{
				rpacket_t r = (rpacket_t)_msg;
				m->_on_packet((datasocket_t)r->ptr,r);
				rpacket_destroy(&r);
			}
			break;
		case MSG_NEW_CONNECTION:
			{
				m->_on_new_connection((datasocket_t)_msg->ptr);
				destroy_msg(&_msg);
			}
			break;
		case MSG_DISCONNECTED:
			{
				datasocket_t s = (datasocket_t)_msg->ptr;
				m->_on_connection_disconnect(s,s->close_reason);
				destroy_msg(&_msg);
			}
			break;
	}
}

void start_msg_loop(msg_loop_t m,netservice_t s)
{
	msg_t _msg = NULL;
	uint32_t last_sync = GetCurrentMs();
	while(0 == m->stop)
	{
		_msg = net_peek_msg(s,10);
		if(_msg)
			dispatch_msg(m,_msg);
		uint32_t now_tick = GetCurrentMs();
		if(now_tick - last_sync >= 50)
		{
			last_sync = now_tick;
			mq_flush();
		}
	}
}

void stop_msg_loop(msg_loop_t m)
{
	m->stop = 1;	
}

void destroy_msg_loop(msg_loop_t *m)
{
	free(*m);
	*m = NULL;
}
