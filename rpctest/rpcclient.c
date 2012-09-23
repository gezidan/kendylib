#include <stdio.h>
#include "SocketWrapper.h"
#include "SysTime.h"
#include "KendyNet.h"
#include "Connector.h"
#include "Connection.h"
#include "link_list.h"
#include "co_sche.h"

struct channel
{
	struct connection *c;
	spin_lock_t mtx;
	struct link_list   *send_list;
	struct block_queue *msgQ;
};

struct channel *g_channel = NULL;
allocator_t wpacket_allocator = NULL;
thread_t logic_thread;
sche_t g_sche = NULL;

//function for logic thread
int32_t send_packet(struct channel *c,wpacket_t w)
{
	spin_lock(c->mtx);
	if(c->c == NULL)
	{
		spin_unlock(c->mtx);
		wpacket_destroy(&w);
		return -1;
	}
	LINK_LIST_PUSH_BACK(c->send_list,w);
	spin_unlock(c->mtx);
	return 0;
}

rpacket_t peek_msg(struct channel *c,uint32_t timeout)
{
	rpacket_t msg = NULL;
	BLOCK_QUEUE_POP(c->msgQ,&msg,timeout);
	return msg;		
}

void *test_coro_fun(void *arg)
{
	coro_t co = get_current_coro();
	while(1)
	{
		wpacket_t wpk = wpacket_create(0,wpacket_allocator,64,0);
		wpacket_write_uint32(wpk,(int32_t)co);
		//uint32_t sys_t = GetSystemMs();
		//wpacket_write_uint32(wpk,sys_t);
		//wpacket_write_string(wpk,"hello kenny");
		send_packet(g_channel,wpk);
		//send and block until the response from rpc server
		coro_block(co);
	}
}

void *logic_routine(void *arg)
{
	while(1)
	{
		rpacket_t rpk = peek_msg(g_channel,50);
		if(rpk)
		{
			coro_t co = (coro_t)rpacket_read_uint32(rpk);
			coro_wakeup(co);
			rpacket_destroy(&rpk);
		}
		sche_schedule(g_sche);	
	}
}


//function for io thread
struct channel *channel_create(struct connection *con)
{
	struct channel *c = calloc(1,sizeof(*c));
	c->c = con;
	c->mtx = spin_create();
	c->send_list = LINK_LIST_CREATE();
	c->msgQ = BLOCK_QUEUE_CREATE();
	return c;
}


void push_msg(struct channel *c,rpacket_t r)
{
	BLOCK_QUEUE_PUSH(c->msgQ,r);
}

void process_send(struct channel *c)
{
	spin_lock(c->mtx);
	link_list_swap(c->c->send_list,c->send_list);
	spin_unlock(c->mtx);
	connection_send(c->c,NULL,NULL);
}


void on_process_packet(struct connection *c,rpacket_t r)
{
	struct channel *_channel = (struct channel*)c->custom_ptr;
	push_msg(_channel,r);	
}


void on_channel_disconnect(struct connection *c,int32_t reason)
{
	struct channel *_channel = (struct channel*)c->custom_ptr;
	HANDLE sock = c->socket;
	if(0 == connection_destroy(&c))
	{
		ReleaseSocketWrapper(sock);
		spin_lock(_channel->mtx);
		_channel->c = NULL;
		spin_unlock(_channel->mtx);		
	}	
}

void on_connect_callback(HANDLE s,const char *ip,int32_t port,void *ud)
{
	HANDLE *engine = (HANDLE*)ud;
	struct connection *c;
	if(s == -1)
	{
		printf("%d,Á¬½Óµ½:%s,%d,Ê§°Ü\n",s,ip,port);
	}
	else
	{
		
		setNonblock(s);
		c = connection_create(s,0,0,on_process_packet,on_channel_disconnect);
		printf("%d,Á¬½Óµ½:%s,%d,³É¹¦\n",s,ip,port);
		Bind2Engine(*engine,s,RecvFinish,SendFinish);
		//create channel and create logic thread
		g_channel = channel_create(c);
		c->custom_ptr = g_channel;
		connection_start_recv(c);
	}
}

int32_t main(int32_t argc,char **argv)
{	
	HANDLE engine;
	
	const char *ip = argv[1];
	uint32_t port = atoi(argv[2]);
	int32_t client_count = atoi(argv[3]);
	signal(SIGPIPE,SIG_IGN);
	if(InitNetSystem() != 0)
	{
		printf("Init error\n");
		return 0;
	}
	wpacket_allocator = (allocator_t)create_block_obj_allocator(0,sizeof(struct wpacket));		
	
	int32_t ret;
	int32_t i = 0;
	uint32_t send_interval = 8;
	uint32_t send_tick = 0;
	wpacket_t wpk;

	init_clients();
	engine = CreateEngine();
	con =  connector_create();
	for( ; i < client_count;++i)
	{
		ret = connector_connect(con,ip,port,on_connect_callback,&engine,1000*20);
		sleepms(1);
	}
	while(1)
	{
		connector_run(con,1);
		EngineRun(engine,1);
		if(g_channel)
			process_send(g_channel);
	}
	return 0;
}
