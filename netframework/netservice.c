#include "netservice.h"
#include "msg.h"
#include "datasocket.h"
#include "wpacket.h"
#include "rpacket.h"
#include "mq.h"
#include "SysTime.h"
#include "double_link.h"

extern struct socket_wrapper* GetSocketByHandle(HANDLE);
extern int32_t      ReleaseSocketWrapper(HANDLE);

static void timeout_check(TimingWheel_t t,void *arg,uint32_t now)
{
	datasocket_t s = (datasocket_t)arg;
	if(now > s->c->last_recv && now - s->c->last_recv >= s->c->recv_timeout)
	{
		//超时了,关闭套接口
		struct socket_wrapper *sw = GetSocketByHandle(s->c->socket);
		if(sw)
		{
			double_link_remove((struct double_link_node*)sw);
			ReleaseSocketWrapper(s->c->socket);	
			//通知上层，连接超时关闭
			s->close_reason = -3;
			s->is_close = 1;
			msg_t _msg = create_msg(s,MSG_DISCONNECTED);
			mq_push(s->e->service->mq_out,(list_node*)_msg);
		}
	}else
	{
		RegisterTimer(t,s->c->wheelitem,500);
	}
	
	//检测是否有发送阻塞
	wpacket_t w = (wpacket_t)link_list_head(s->c->send_list);
	if(w)
	{
		if(now > w->send_tick && now - w->send_tick >= s->c->send_timeout)
		{
			//发送队列队首包超过了15秒任然没有发出去,通知上层,发送阻塞
			msg_t _msg = create_msg(s,MSG_SEND_BLOCK);
			mq_push(s->e->service->mq_out,(list_node*)_msg);
		}
	}
}

static void on_process_msg(struct engine_struct *e,msg_t _msg)
{
	switch(_msg->type)
	{
		case MSG_ACTIVE_CLOSE:
			{
				datasocket_t s = (datasocket_t)_msg->ptr;
				UnRegisterTimer(e->timingwheel,s->c->wheelitem);
				connection_active_close(s->c);
			}
			break;
		case MSG_NEW_CONNECTION:
			{
				datasocket_t s = (datasocket_t)_msg->ptr;
				Bind2Engine(e->engine,s->c->socket,RecvFinish,SendFinish);
				s->c->last_recv = GetSystemMs();
				connection_start_recv(s->c);
			}
			break;
		case MSG_SET_RECV_TIMEOUT:
		case MSG_SET_SEND_TIMEOUT:
			{
				datasocket_t s = (datasocket_t)_msg->ptr;
				if(!s->c->wheelitem)
				{
					s->c->wheelitem = CreateWheelItem((void*)s,timeout_check);
					RegisterTimer(e->timingwheel,s->c->wheelitem,500);
				}
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
		connection_send(s->c,w,NULL);
	ref_decrease(&(s)->_refbase);
}

static void on_process_packet(struct connection *c,rpacket_t r)
{
	datasocket_t s = c->custom_ptr;
	r->ptr = s;
	mq_push(s->e->service->mq_out,(list_node*)r);	
}

static void on_socket_disconnect(struct connection *c,int32_t reason)
{
	datasocket_t s = c->custom_ptr;
	if(reason == -1)
	{
		UnRegisterTimer(s->e->timingwheel,c->wheelitem);
		//通知上层，连接被动断开
		s->close_reason = reason;
		s->is_close = 1;
		msg_t _msg = create_msg(s,MSG_DISCONNECTED);
		mq_push(s->e->service->mq_out,(list_node*)_msg);
	}
}



static void *mainloop(void *arg)
{	
	struct engine_struct *e = (struct engine_struct*)arg;
	uint32_t last_sync = GetCurrentMs();
	while(0 == e->service->stop)
	{
		msg_t _msg = NULL;
		while(_msg = (msg_t)mq_pop(e->mq_in,0))
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
				on_process_msg(e,_msg);
				destroy_msg(&_msg);
			}
		}
		//执行超时检测
		UpdateWheel(e->timingwheel,GetCurrentMs());
		//////////
		EngineRun(e->engine,1);
		//冲刷mq
		mq_flush();

	}
}

static void accept_callback(HANDLE s,void *ud)
{
	netservice_t service = (netservice_t)ud;
	struct connection *c = connection_create(s,0,MUTIL_THREAD,on_process_packet,on_socket_disconnect);
	setNonblock(s);
	//随机选择一个engine
	int32_t index = rand()%service->engine_count;
	struct engine_struct *e = &(service->engines[index]);
	datasocket_t data_s = create_datasocket(e,c,e->mq_in);
	//c->last_recv = GetSystemMs();
	//c->timeout = 1*60*1000;
	//c->wheelitem = CreateWheelItem((void*)data_s,timeout_check);
	//通知上层，一个新连接到来
	msg_t _msg = create_msg(data_s,MSG_NEW_CONNECTION);
	mq_push(service->mq_out,(list_node*)_msg);
	//通知engine新连接到来
	_msg = create_msg(data_s,MSG_NEW_CONNECTION);
	mq_push(e->mq_in,(list_node*)_msg);	
	mq_flush();
}

static void *_Listen(void *arg)
{
	netservice_t service = (netservice_t)arg;
	while(0 == service->stop)
		acceptor_run(service->_acceptor,100);
	return NULL;
}


static void mq_item_destroyer(void *ptr)
{
	msg_t _msg = (msg_t)ptr;
	if(_msg->type == MSG_RPACKET)
		rpacket_destroy((rpacket_t*)&_msg);
	else if(_msg->type == MSG_WPACKET)
		wpacket_destroy((wpacket_t*)&_msg);
	else
		destroy_msg(&_msg);
}

netservice_t create_net_service(uint32_t thread_count)
{
	if(thread_count == 0)
		thread_count = 1;
	if(thread_count > MAX_ENGINES)
		thread_count = MAX_ENGINES;
	
	netservice_t s = (netservice_t)calloc(1,sizeof(*s));
	
	s->engine_count = thread_count;
	s->mq_out = create_mq(512,mq_item_destroyer);
	s->thread_listen = create_thread(1);//joinable
	s->_acceptor = create_acceptor();
	
	uint32_t i = 0;
	for( ;i < thread_count; ++i)
	{
		s->engines[i].mq_in = create_mq(512,mq_item_destroyer);
		s->engines[i].engine = CreateEngine();
		s->engines[i].thread_engine = create_thread(1);//joinable
		s->engines[i].service = s;
		s->engines[i].timingwheel = CreateTimingWheel(500,30*1000);
		thread_run(mainloop,&s->engines[i]);//启动线程
	}
	thread_run(_Listen,s);//启动listener线程
	return s;	
}

void stop_net_service(netservice_t s)
{
	if(s->stop == 0)
	{
		s->stop = 1;
		//等待各线程结束
		thread_join(s->thread_listen);
		uint32_t i = 0;
		for( ;i < s->engine_count; ++i)
			thread_join(s->engines[i].thread_engine);
	}
}

void destroy_net_service(netservice_t *s)
{
	netservice_t _s = *s;
	stop_net_service(_s);
	destroy_acceptor(&_s->_acceptor);
	destroy_thread(&_s->thread_listen);
	uint32_t i = 0;
	for( ;i < _s->engine_count; ++i)
	{
		DestroyTimingWheel(&_s->engines[i].timingwheel);
		destroy_thread(&_s->engines[i].thread_engine);
		CloseEngine(_s->engines[i].engine);
		destroy_mq(&_s->engines[i].mq_in);
	}
	destroy_mq(&_s->mq_out);
	free(_s);
	*s = NULL;
}

//从网络服务队列中获取到来的消息
msg_t net_peek_msg(netservice_t s,uint32_t ms)
{
	return (msg_t)mq_pop(s->mq_out,ms);
}

//添加一个网络监听
HANDLE net_add_listener(netservice_t s,const char *ip,uint32_t port)
{
	return add_listener(s->_acceptor,ip,port,accept_callback,s);
}
//关闭一个网络监听
void net_rem_listener(netservice_t s,HANDLE h)
{
	rem_listener(s->_acceptor,h);
}

