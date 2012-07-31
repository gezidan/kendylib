#include <stdio.h>
#include "SocketWrapper.h"
#include "SysTime.h"
#include "KendyNet.h"
#include "Connector.h"
#include "Connection.h"

static int32_t connect_count = 0;
uint32_t packet_recv = 0;
uint32_t packet_send = 0;
uint32_t send_request = 0;
uint32_t tick = 0;
uint32_t now = 0;
uint32_t bf_count = 0;
#define MAX_CLIENT 1000
static struct connection *clients[MAX_CLIENT];
uint32_t last_recv = 0;
uint32_t ava_interval = 0;
uint32_t s_p = 0;
uint32_t recv_count = 0;
void init_clients()
{
	int32_t i = 0;
	for(; i < MAX_CLIENT;++i)
		clients[i] = 0;
}

void add_client(struct connection *c)
{
	int32_t i = 0;
	for(; i < MAX_CLIENT; ++i)
	{
		if(clients[i] == 0)
		{
			clients[i] = c;
			break;
		}
	}
	ReleaseSocketWrapper(c->socket);
	connection_destroy(&c);
}

void remove_client(struct connection *c,int32_t reason)
{
	int32_t i = 0;
	for(; i < MAX_CLIENT; ++i)
	{
		if(clients[i] == c)
		{
			clients[i] = 0;
			break;
		}
	}
	HANDLE sock = c->socket;
	if(0 == connection_destroy(&c))
	{
		ReleaseSocketWrapper(sock);
	}	
}

connector_t con = NULL;
uint32_t iocp_count = 0;

void on_process_packet(struct connection *c,rpacket_t r)
{
	uint32_t s = rpacket_read_uint32(r);
	uint32_t t;
	if(s == c->socket)
	{
		t = rpacket_read_uint32(r);
		uint32_t sys_t = GetSystemMs();
		ava_interval += (sys_t - t);
		ava_interval /= 2;
	}
	++packet_recv;
	rpacket_destroy(&r);
	
}

void on_connect_callback(HANDLE s,const char *ip,int32_t port,void *ud)
{
	HANDLE *engine = (HANDLE*)ud;
	struct connection *c;
	wpacket_t wpk;
	++connect_count;
	if(s == -1)
	{
		printf("%d,连接到:%s,%d,失败\n",s,ip,port);
	}
	else
	{
		
		setNonblock(s);
		c = connection_create(s,0,0,on_process_packet,remove_client);
		printf("%d,连接到:%s,%d,成功\n",s,ip,port);
		add_client(c);
		Bind2Engine(*engine,s,RecvFinish,SendFinish);
		wpk = wpacket_create(0,NULL,64,0);
		wpacket_write_uint32(wpk,(uint32_t)s);
		uint32_t sys_t = GetSystemMs();
		wpacket_write_uint32(wpk,sys_t);
		wpacket_write_string(wpk,"hello kenny");
		connection_send(c,wpk);
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
	
	int32_t ret;
	int32_t i = 0;
	uint32_t send_interval = 8;
	uint32_t send_tick = 0;
	wpacket_t wpk;
	//init_wpacket_pool(50000);
	//init_rpacket_pool(100000);
	//buffer_init_maxbuffer_size(2000);
	//buffer_init_64(2000);

	init_clients();
	engine = CreateEngine();
	con =  connector_create();
	for( ; i < client_count;++i)
	{
		ret = connector_connect(con,ip,port,on_connect_callback,&engine,1000*20);
		usleep(1);
	}
	while(1)
	{
		connector_run(con,1);
		EngineRun(engine,50);
		now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("recv:%u,send:%u,s_req:%u,ava_interval:%u\n",packet_recv,packet_send,send_request,ava_interval);
			tick = now;
			packet_recv = 0;
			packet_send = 0;
			send_request = 0;
			ava_interval = 0;
		}
		if(ava_interval > 200)
			send_interval = 200;
		else
			send_interval = 8;
		if(now - send_tick > send_interval)
		{
			send_tick = now;
			for(i = 0; i < client_count; ++i)
			{
				if(clients[i])
				{
					wpk = wpacket_create(0,NULL,64,0);
					wpacket_write_uint32(wpk,clients[i]->socket);
					uint32_t sys_t = GetSystemMs();
					wpacket_write_uint32(wpk,sys_t);
					wpacket_write_string(wpk,"hello kenny");
					connection_send(clients[i],wpk);
				}
			}
		}
	}
	return 0;
}
