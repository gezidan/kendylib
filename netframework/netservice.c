#include "netservice.h"
#include "msg.h"
#include "datasocket.h"
#include "wpacket.h"
#include "rpacket.h"

static void on_process_msg(struct engine_struct *e,msg_t _msg)
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
		//通知上层，连接被动断开
		s->close_reason = reason;
		msg_t _msg = create_msg(s,MSG_DISCONNECTED);
		mq_push(s->e->service->mq_out,(list_node*)_msg);
	}
	release_datasocket(&s);
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
		//////////
		
		uint32_t now = GetCurrentMs();
		//冲刷mq
		if(now - last_sync >= 50)
		{
			mq_force_sync(e->service->mq_out);
			last_sync = now;
		}
		EngineRun(e->engine,50);
	}
}

static void accept_callback(HANDLE s,void *ud)
{
	netservice_t service = (netservice_t)ud;
	struct connection *c = connection_create(s,0,SINGLE_THREAD,on_process_packet,on_socket_disconnect);
	setNonblock(s);
	//随机选择一个engine
	int32_t index = rand()%service->engine_count;
	struct engine_struct *e = &(service->engines[index]);
	datasocket_t data_s = create_datasocket(e,c,e->mq_in);
	//通知上层，一个新连接到来
	msg_t _msg = create_msg(data_s,MSG_NEW_CONNECTION);
	mq_push(service->mq_out,(list_node*)_msg);
	connection_start_recv(c);
	Bind2Engine(e->engine,s,RecvFinish,SendFinish);
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
	s->mq_out = create_mq(4096,mq_item_destroyer);
	s->thread_listen = create_thread(1);//joinable
	s->_acceptor = create_acceptor();
	
	uint32_t i = 0;
	for( ;i < thread_count; ++i)
	{
		s->engines[i].mq_in = create_mq(4096,mq_item_destroyer);
		s->engines[i].engine = CreateEngine();
		s->engines[i].thread_engine = create_thread(1);//joinable
		s->engines[i].service = s;
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

