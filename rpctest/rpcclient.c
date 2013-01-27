#include <stdio.h>
#include "net/SocketWrapper.h"
#include "util/SysTime.h"
#include "net/KendyNet.h"
#include "net/Connector.h"
#include "net/Connection.h"
#include "util/link_list.h"
#include "co_sche.h"
#include "util/thread.h"
#include "util/mq.h"
#include "net/common_define.h"
//#include "spinlock.h"
struct channel
{
	struct connection *c;
	mq_t   send_list;
	mq_t   msgQ;
};

struct channel *g_channel = NULL;
thread_t logic_thread;
sche_t g_sche = NULL;
uint32_t call_count = 0;
allocator_t wpacket_allocator = NULL;
atomic_32_t wpacket_count = 0;
atomic_32_t rpacket_count = 0;
atomic_32_t buf_count = 0;

//function for logic thread
static inline int32_t send_packet(struct channel *c,wpacket_t w)
{
	mq_push(c->send_list,(struct list_node*)w);
	return 0;
}

static inline rpacket_t peek_msg(struct channel *c,uint32_t timeout)
{
	return (rpacket_t)mq_pop(c->msgQ,timeout);		
}


static inline int sum(int32_t arg1,int32_t arg2)
{
	coro_t co = get_current_coro();
	wpacket_t wpk = wpacket_create(MUTIL_THREAD,wpacket_allocator,64,0);
	wpacket_write_uint32(wpk,(int32_t)co);
	wpacket_write_string(wpk,"sum");
	wpacket_write_uint32(wpk,arg1);
	wpacket_write_uint32(wpk,arg2);
	send_packet(g_channel,wpk);
	//send and block until the response from rpc server
	coro_block(co);
	int ret = rpacket_read_uint32(co->rpc_response);
	rpacket_destroy(&co->rpc_response);
	return ret;
}

static inline int product(int32_t arg1,int32_t arg2)
{
	coro_t co = get_current_coro();
	wpacket_t wpk = wpacket_create(MUTIL_THREAD,NULL,64,0);
	wpacket_write_uint32(wpk,(int32_t)co);
	wpacket_write_string(wpk,"product");
	wpacket_write_uint32(wpk,arg1);
	wpacket_write_uint32(wpk,arg2);	
	send_packet(g_channel,wpk);
	//send and block until the response from rpc server
	coro_block(co);
	int ret = rpacket_read_uint32(co->rpc_response);
	rpacket_destroy(&co->rpc_response);
	return ret;
}


void *test_coro_fun1(void *arg)
{
	coro_t co = get_current_coro();
	while(1)
	{
		int32_t arg1 = rand()%4096;
		int32_t arg2 = rand()%4096;
		if(arg1+arg2 != sum(arg1,arg2))
		{
			printf("rpc error\n");
			exit(0);
		}	
		++call_count;
	}
}

void *test_coro_fun2(void *arg)
{
	coro_t co = get_current_coro();
	while(1)
	{
		int32_t arg1 = rand()%512;
		int32_t arg2 = rand()%512;
		if(arg1*arg2 != product(arg1,arg2))
		{
			printf("rpc error\n");
			exit(0);
		}
		++call_count;
	}
}

static inline void  sche_idel(void *arg)
{
	rpacket_t rpk = peek_msg(g_channel,50);
	if(rpk)
	{
		coro_t co = (coro_t)rpacket_read_uint32(rpk);
		co->rpc_response = rpk;
		coro_wakeup(co);
	}
}


void *logic_routine(void *arg)
{
	uint32_t tick = GetSystemMs();
	while(1)
	{
		rpacket_t rpk;
		while(rpk = peek_msg(g_channel,0))
		{
			coro_t co = (coro_t)rpacket_read_uint32(rpk);
			co->rpc_response = rpk;
			coro_wakeup(co);
		}
		uint32_t now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("call_count:%u\n",(call_count*1000)/(now-tick));
			tick = now;
			call_count = 0;
		}
		sche_schedule(g_sche);
	}
}


//function for io thread
struct channel *channel_create(struct connection *con)
{
	struct channel *c = calloc(1,sizeof(*c));
	c->c = con;
	c->send_list = create_mq(8192,MQ_DEFAULT_ITEM_DESTROYER);
	c->msgQ = create_mq(8192,MQ_DEFAULT_ITEM_DESTROYER);
	return c;
}


static inline void push_msg(struct channel *c,rpacket_t r)
{
	mq_push(c->msgQ,(struct list_node*)r);
	
}

static inline void process_send(struct channel *c)
{
	mq_swap(c->send_list,c->c->send_list,0);
	connection_send(c->c,NULL,NULL);
}


void on_process_packet(struct connection *c,rpacket_t r)
{
	struct channel *_channel = (struct channel*)c->usr_data;
	push_msg(_channel,r);	
}


void on_channel_disconnect(struct connection *c,int32_t reason)
{
	struct channel *_channel = (struct channel*)c->usr_data;
	HANDLE sock = c->socket;
	if(0 == connection_destroy(&c))
	{
		ReleaseSocketWrapper(sock);
		//spin_lock(_channel->mtx);
		_channel->c = NULL;
		//spin_unlock(_channel->mtx);		
	}	
}

connector_t con = NULL;

void on_connect_callback(HANDLE s,const char *ip,int32_t port,void *ud)
{
	HANDLE *engine = (HANDLE*)ud;
	struct connection *c;
	if(s == -1)
	{
		printf("connect failed\n");
	}
	else
	{
		
		setNonblock(s);
		c = connection_create(s,0,MUTIL_THREAD,on_process_packet,on_channel_disconnect);
		printf("connect successed\n");
		Bind2Engine(*engine,s,RecvFinish,SendFinish);
		//create channel and create logic thread
		g_channel = channel_create(c);
		c->usr_data = g_channel;
		
		g_sche = sche_create(50000,4096,sche_idel,NULL);
		
		int i = 0;
		for(; i < 50000; ++i)
		{
			if(i%2 == 0)
				sche_spawn(g_sche,test_coro_fun1,NULL);
			else
				sche_spawn(g_sche,test_coro_fun2,NULL);
		}
		thread_run(logic_routine,NULL);
		connection_start_recv(c);
		
		
		
	}
}

int32_t main(int32_t argc,char **argv)
{	
	HANDLE engine;
	init_system_time(10);
	init_mq_system();
	const char *ip = argv[1];
	uint32_t port = atoi(argv[2]);
	signal(SIGPIPE,SIG_IGN);
	if(InitNetSystem() != 0)
	{
		printf("Init error\n");
		return 0;
	}		
	
	int32_t ret;
	wpacket_t wpk;
	engine = CreateEngine();
	con =  connector_create();
	ret = connector_connect(con,ip,port,on_connect_callback,&engine,1000*20);
	while(1)
	{
		connector_run(con,1);
		EngineRun(engine,50);
		if(g_channel)
			process_send(g_channel);	
	}
	return 0;
}
