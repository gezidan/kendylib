#include "netservice.h"


static void on_process_msg(netservice_t service,msg_t _msg)
{
	switch(_msg->type)
	{
		case MSG_ACTIVE_CLOSE:
			{
				datasocket_t s = (datasocket_t)_msg->ptr;
				connection_active_close(s->c);
			}
			break;
		default:
			break;
	}
}

static void on_process_send(datasocket_t s,wpacket_t w)
{
	if(s->is_close)
		wpacket_destroy(&w);//连接已经关闭，不再需要发送
	else
		connection_send(s->c,w);
	ref_decrease(&(*s)->_refbase);
}

static void on_process_packet(struct connection *c,rpacket_t r)
{
	datasocket_t s = c->custom_ptr;
	r->ptr = s;
	mq_push(s->service->mq_out,r);	
}

static void on_socket_disconnect(struct connection *c,int32_t reason)
{
	datasocket_t s = c->custom_ptr;
	if(reason == -1)
	{
		//通知上层，连接被动断开
		msg_t _msg = create_msg(s,MSG_DISCONNECTED);
		mq_push(s->service->mq_out,_msg);
	}
	release_datasocket(&s);
}

static void *mainloop(void *arg)
{
	netservice_t service = (netservice_t)arg;
	uint32_t last_sync = GetCurrentMs();
	while(0 == service->stop)
	{
		msg_t _msg = NULL;
		while(_msg = mq_pop(service->mq_in,0))
		{
			if(_msg->type == MSG_WPACKET)
			{
				//是一个需要发送的数据包
				wpacket_t wpk = (wpacket_t)_msg;
				datasocket_t s = (datasocket_t)wpk->ptr;
				on_process_send(s,wpk);
			}
			else
			{
				//处理消息
				on_process_msg(service,_msg);
				destroy_msg(&_msg);
			}
		}
		//执行超时检测
		//////////
		
		uint32_t now = GetCurrentMs();
		//冲刷mq
		if(now - last_sync >= 50)
		{
			mq_force_sync(service->mq_out);
			last_sync = now;
		}
		EngineRun(service->engine,50);
	}
}

static void accept_callback(HANDLE s,void *ud)
{
	netservice_t service = (netservice_t)ud;
	struct connection *c = connection_create(s,0,SINGLE_THREAD,on_process_packet,on_socket_disconnect);
	setNonblock(s);
	datasocket_t data_s = create_datasocket(service,c,service->mq_in);
	//通知上层，一个新连接到来
	msg_t _msg = create_msg(data_s,MSG_NEW_CONNECTION);
	mq_push(service->mq_out,_msg);
	connection_start_recv(c);
	Bind2Engine(service->engine,s,RecvFinish,SendFinish);
}

static void *_Listen(void *arg)
{
	netservice_t service = (netservice_t)arg;
	while(0 == service->stop)
		acceptor_run(service->_acceptor,100);
	return NULL;
}
