#include "netservice.h"
#include "msg.h"
#include "datasocket.h"
#include "util/mq.h"
#include "util/SysTime.h"
#include "util/double_link.h"
#include "util/block_obj_allocator.h"
#include "net/Connector.h"

extern struct socket_wrapper* GetSocketByHandle(HANDLE);
extern int32_t      ReleaseSocketWrapper(HANDLE);


static int8_t is_init = 0;
static allocator_t rpacket_allocator = NULL;
static allocator_t wpacket_allocator = NULL;

#define MQ_SYNC_SIZE 64                 //消息队列冲刷的阀值
#define SYS_TICKER 5                    //系统时间更新的间隔(单位ms)
#define ENGINE_RUN_TIME 2               //网络循环的运行时间(单位ms)
#define WHEEL_TICK 1000                 //时间轮的时间间隔(单位ms)

int32_t init_net_service()
{
	if(0 == is_init)
	{

		if(InitNetSystem() != 0)
			return -1;
		is_init = 1;
		init_system_time(SYS_TICKER);
		signal(SIGPIPE,SIG_IGN);
		init_mq_system();
		rpacket_allocator = (allocator_t)create_block_obj_allocator(MUTIL_THREAD,sizeof(struct rpacket));
		wpacket_allocator = (allocator_t)create_block_obj_allocator(MUTIL_THREAD,sizeof(struct wpacket));
		return 0;
	}
	return -1;
}

wpacket_t    get_wpacket(uint32_t size)
{
	return wpacket_create(MUTIL_THREAD,wpacket_allocator,size,0);
}

wpacket_t    get_wpacket_by_rpacket(rpacket_t r)
{
	return wpacket_create_by_rpacket(wpacket_allocator,r);
}

struct st_connd
{
	uint32_t idx;
	uint32_t timestamp;
};

static inline void on_process_packet(struct connection *c,rpacket_t r)
{
	datasocket_t s = (datasocket_t)c->usr_data;
	r->usr_data = (uint64_t)s;
	mq_push(s->e->service->mq_out,(list_node*)r);
	//printf("recv packet\n");
}

static void on_socket_disconnect(struct connection *c,int32_t reason);

static struct connection *get_free_connection(struct engine_struct *e,connd_t *connd)
{
	uint32_t i = 1;
	if(0 == e->con_free_size)
	{
		//扩大容量
		uint32_t new_size = e->con_pool_size*2;
		struct con_pair *new_pool = calloc(new_size,sizeof(*new_pool));
		if(!new_pool)
			return NULL;
		memcpy(new_pool,e->con_pool,sizeof(*new_pool)*e->con_pool_size);
		for(i = e->con_pool_size; i < new_size; ++i)
			e->con_pool[i].c = connection_create(-1,0,MUTIL_THREAD,on_process_packet,on_socket_disconnect);
		i =	e->con_pool_size;
		e->con_free_size = new_size - e->con_pool_size;
		e->con_pool_size = new_size;
	}

	for( ; i < e->con_pool_size; ++i)
	{
		if(e->con_pool[i].c && 0 == e->con_pool[i].timestamp)
		{
			e->con_pool[i].timestamp = GetSystemMs();
			struct st_connd *tmp = (struct st_connd*)connd;
			tmp->idx = i;
			tmp->timestamp = e->con_pool[i].timestamp;
			--e->con_free_size;
			return e->con_pool[i].c;
		}
	}
	return NULL;
}

static inline struct connection *get_connection(struct engine_struct *e,connd_t connd)
{
	struct st_connd *tmp = (struct st_connd*)&connd;
	if(tmp->idx >= 1 && tmp->idx < e->con_pool_size && tmp->timestamp == e->con_pool[tmp->idx].timestamp)
		return e->con_pool[tmp->idx].c;
	return NULL;
}

static inline void free_connection(struct engine_struct *e,connd_t connd)
{
	struct st_connd *tmp = (struct st_connd*)&connd;
	if(tmp->idx >= 1 && tmp->idx < e->con_pool_size && tmp->timestamp == e->con_pool[tmp->idx].timestamp)
	{
		struct connection *c = e->con_pool[tmp->idx].c;
		ReleaseSocketWrapper(c->socket);
		wpacket_t w;
		while((w = LINK_LIST_POP(wpacket_t,c->send_list))!=NULL)
			wpacket_destroy(&w);
		buffer_release(&c->unpack_buf);
		buffer_release(&c->next_recv_buf);
		c->next_recv_pos = 0;
		c->unpack_pos = 0;
		c->unpack_size = 0;
		c->is_close = 0;
		c->send_timeout = c->recv_timeout = 0;
		c->recv_overlap.isUsed = c->send_overlap.isUsed = 0;
		if(c->wheelitem)
			DestroyWheelItem(&(c->wheelitem));
		e->con_pool[tmp->idx].timestamp = 0;
		++e->con_free_size;
	}
}


static void timeout_check(TimingWheel_t t,void *arg,uint32_t now)
{

	datasocket_t s = (datasocket_t)arg;
	struct connection *c = get_connection(s->e,s->c);
	if(NULL == c)
		return;
	if(c->recv_timeout > 0 && now > c->last_recv && now - c->last_recv >= c->recv_timeout)
	{
		//超时了,关闭套接口
		free_connection(s->e,s->c);
		//通知上层，连接超时关闭
		s->close_reason = -3;
		s->is_close = 1;
		msg_t _msg = create_msg((uint64_t)s,MSG_DISCONNECTED);
		mq_push(s->e->service->mq_out,(list_node*)_msg);
		return;
	}else
	{
		RegisterTimer(t,c->wheelitem,WHEEL_TICK);
	}

	//检测是否有发送阻塞
	if(c->send_timeout > 0)
	{
		wpacket_t w = (wpacket_t)link_list_head(c->send_list);
		if(w)
		{
			if(now > w->send_tick && now - w->send_tick >= c->send_timeout)
			{
				//发送队列队首包超过了15秒任然没有发出去,通知上层,发送阻塞
				msg_t _msg = create_msg((uint64_t)s,MSG_SEND_BLOCK);
				mq_push(s->e->service->mq_out,(list_node*)_msg);

			}
		}
	}
}

static void on_process_msg(struct engine_struct *e,msg_t _msg)
{
	switch(_msg->type)
	{
		case MSG_ACTIVE_CLOSE:
			{
				datasocket_t s = (datasocket_t)_msg->usr_data;
				connd_t con = s->c;
				struct connection *c = get_connection(e,con);
				if(c)
					connection_active_close(c);
			}
			break;
		case MSG_NEW_CONNECTION:
			{
				HANDLE s = (HANDLE)_msg->usr_data;
				setNonblock(s);
				connd_t con = 0;
				struct connection *c = get_free_connection(e,&con);
				if(!c)
				{
					ReleaseSocketWrapper(s);
					printf("empty free connection\n");
					exit(0);
					return;
				}
				if(0!=Bind2Engine(e->engine,s,RecvFinish,SendFinish,NULL))
				{
					ReleaseSocketWrapper(s);
					connection_destroy(&c);
				}
				else
				{
					c->rpacket_allocator = rpacket_allocator;
					c->socket = s;
					datasocket_t data_s = create_datasocket(e,con,e->mq_in);
					c->usr_data = (uint64_t)data_s;
					//通知上层，一个新连接到来
					msg_t _msg = create_msg((uint64_t)data_s,MSG_NEW_CONNECTION);
					mq_push(e->service->mq_out,(list_node*)_msg);
					mq_flush();
					c->last_recv = GetSystemMs();
					int32_t ret = connection_start_recv(c);
					if(-10 == ret)
					{
						printf("connection_start_recv error:%d\n",ret);
						exit(0);
					}
				}
			}
			break;

		case MSG_SET_RECV_TIMEOUT:
		case MSG_SET_SEND_TIMEOUT:
			{
				datasocket_t s = (datasocket_t)_msg->usr_data;
				struct connection *c = get_connection(e,s->c);
				if(c)
				{
					c->recv_timeout = s->recv_timeout;
					c->send_timeout = s->send_timeout;
					if(!c->wheelitem)
					{
						c->wheelitem = CreateWheelItem((void*)s,timeout_check,NULL);
						RegisterTimer(e->timingwheel,c->wheelitem,WHEEL_TICK);
					}
				}
				ref_decrease(&s->_refbase);
			}
			break;
		default:
			break;
	}
}

static inline void on_process_send(struct engine_struct *e,wpacket_t w)
{
	connd_t con = w->usr_data;
	struct connection *c = get_connection(e,con);
	if(NULL == c)
		wpacket_destroy(&w);
	else
		connection_send(c,w,NULL);
}

static void on_socket_disconnect(struct connection *c,int32_t reason)
{
	datasocket_t s = (datasocket_t)c->usr_data;
	UnRegisterTimer(c->wheelitem);
	free_connection(s->e,s->c);
	if(reason == -1)
	{

		//通知上层，连接被动断开
		s->close_reason = reason;
		s->is_close = 1;
		msg_t _msg = create_msg((uint64_t)s,MSG_DISCONNECTED);
		mq_push(s->e->service->mq_out,(list_node*)_msg);
		mq_flush();
	}
}



static void *mainloop(void *arg)
{
	printf("start io thread\n");
	struct engine_struct *e = (struct engine_struct*)arg;
	//uint32_t last_sync = GetCurrentMs();
	while(0 == e->service->stop)
	{
		msg_t _msg = NULL;
		while((_msg = (msg_t)mq_pop(e->mq_in,0))!=NULL)
		{
			if(_msg->type == MSG_WPACKET)
			{
				//是一个需要发送的数据包
				on_process_send(e,(wpacket_t)_msg);
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
		EngineRun(e->engine,ENGINE_RUN_TIME);
		//冲刷mq
		mq_flush();
	}
	return NULL;
}


static void new_connection(netservice_t service,HANDLE s)
{
	//随机选择一个engine
	int32_t index = rand()%service->engine_count;
	struct engine_struct *e = &(service->engines[index]);
	msg_t _msg = create_msg((uint64_t)s,MSG_NEW_CONNECTION);
	mq_push(e->mq_in,(list_node*)_msg);
	mq_flush();
}

static void accept_callback(HANDLE s,void *ud)
{
	netservice_t service = (netservice_t)ud;
	new_connection(service,s);
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
	if(thread_count > MAX_IO_THREADS)
		thread_count = MAX_IO_THREADS;

	netservice_t s = (netservice_t)calloc(1,sizeof(*s));

	s->engine_count = thread_count;
	s->mq_out = create_mq(MQ_SYNC_SIZE,mq_item_destroyer);
	s->thread_listen = create_thread(THREAD_JOINABLE);//joinable
	s->_acceptor = create_acceptor();

	uint32_t i = 0;
	for( ;i < thread_count; ++i)
	{
		s->engines[i].mq_in = create_mq(MQ_SYNC_SIZE,mq_item_destroyer);
		s->engines[i].engine = CreateEngine();
		s->engines[i].thread_engine = create_thread(THREAD_JOINABLE);//joinable
		s->engines[i].service = s;
		s->engines[i].timingwheel = CreateTimingWheel(WHEEL_TICK,MAX_WHEEL_TIME);
		s->engines[i].con_pool = calloc(INIT_CON_POOL_SIZE,sizeof(*s->engines[i].con_pool));
		s->engines[i].con_pool_size = INIT_CON_POOL_SIZE;
		s->engines[i].con_free_size = INIT_CON_POOL_SIZE-1;
		uint32_t j = 1;
		for( ; j < INIT_CON_POOL_SIZE; ++j)
		{
			s->engines[i].con_pool[j].c = connection_create(-1,0,MUTIL_THREAD,on_process_packet,on_socket_disconnect);
		}
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
		uint32_t j = 1;
		for( ; j < _s->engines[i].con_pool_size; ++j)
			connection_destroy(&_s->engines[i].con_pool[j].c);
		free(_s->engines[i].con_pool);
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

static void connect_callback(HANDLE s,const char *ip,int32_t port,void*ud)
{
	netservice_t service = (netservice_t)ud;
	new_connection(service,s);
}

int32_t net_connect(netservice_t s,const char *ip,uint32_t port)
{
	int32_t ret;
	connector_t con = connector_create();
	ret = connector_connect(con,ip,port,connect_callback,(void*)s,0);
	connector_destroy(&con);
	return ret;
}
