
#include "net/KendyNet.h"
#include "net/Connection.h"
#include <stdio.h>
#include <stdlib.h>
#include "util/thread.h"
#include "net/SocketWrapper.h"
#include "util/SysTime.h"
#include "net/Acceptor.h"
#include <stdint.h>
#include "util/block_obj_allocator.h"
#include <assert.h>
#include "net/common_define.h"
uint32_t packet_recv = 0;
uint32_t packet_send = 0;
uint32_t send_request = 0;
uint32_t tick = 0;
uint32_t now = 0;
uint32_t clientcount = 0;
uint32_t last_send_tick = 0;
allocator_t wpacket_allocator = NULL;
uint32_t total_bytes_recv = 0;

#define MAX_CLIENT 2000
static struct connection *clients[MAX_CLIENT];

void init_clients()
{
	uint32_t i = 0;
	for(; i < MAX_CLIENT;++i)
		clients[i] = 0;
}

void add_client(struct connection *c)
{
	uint32_t i = 0;
	for(; i < MAX_CLIENT; ++i)
	{
		if(clients[i] == 0)
		{
			clients[i] = c;
			break;
		}
	}
}

void send2_all_client(rpacket_t r)
{
	uint32_t i = 0;
	wpacket_t w;
	for(; i < MAX_CLIENT; ++i)
	{
		if(clients[i])
		{
			w = wpacket_create_by_rpacket(wpacket_allocator,r);
			assert(w);
			++send_request;
			connection_send(clients[i],w,NULL);
			//connection_push_packet(clients[i],w,NULL);
		}
	}
}

void remove_client(struct connection *c,int32_t reason)
{
	uint32_t i = 0;
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

void on_process_packet(struct connection *c,rpacket_t r)
{
	//send2_all_client(r);
	wpacket_t w = wpacket_create_by_rpacket(wpacket_allocator,r);
	connection_send(c,w,NULL);	
	++send_request;
	
	total_bytes_recv += rpacket_len(r);
	rpacket_destroy(&r);
	++packet_recv;	
}

void accept_callback(HANDLE s,void *ud)
{
	HANDLE *engine = (HANDLE*)ud;	
	struct connection *c = connection_create(s,0,SINGLE_THREAD,on_process_packet,remove_client);
	add_client(c);
	printf("cli fd:%d\n",s);
	setNonblock(s);
	//发出第一个读请求
	connection_start_recv(c);
	Bind2Engine(*engine,s,RecvFinish,SendFinish);
}


const char *ip;
uint32_t port;


void *_Listen(void *arg)
{
	acceptor_t a = create_acceptor();
	add_listener(a,ip,port,accept_callback,arg);
	while(1)
		acceptor_run(a,100);
	return 0;
}
uint32_t iocp_count = 0; 
int main(int argc,char **argv)
{

	HANDLE engine;
	uint32_t n;
	init_system_time(10);
	ip = argv[1];
	port = atoi(argv[2]);
	signal(SIGPIPE,SIG_IGN);
	if(InitNetSystem() != 0)
	{
		printf("Init error\n");
		return 0;
	}
	wpacket_allocator = (allocator_t)create_block_obj_allocator(SINGLE_THREAD,sizeof(struct wpacket));	

	uint32_t i = 0;
	init_clients();

	engine = CreateEngine();
	thread_run(_Listen,&engine);
	tick = GetSystemMs();
	while(1)
	{
		EngineRun(engine,100);
		now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("recv:%u,send:%u,s_req:%u,total_recv:%u\n",packet_recv,packet_send,send_request,total_bytes_recv/1024/1024);
			tick = now;
			packet_recv = 0;
			packet_send = 0;
			send_request = 0;
			iocp_count = 0;
			total_bytes_recv = 0;
			//printf("w:%d,r:%d,b:%d\n",wpacket_count,rpacket_count,buf_count);
		}
		/*
		if(now - last_send_tick > 10)
		{
			last_send_tick = now;
			for(i=0; i < MAX_CLIENT; ++i)
			{
				if(clients[i])
				{
					connection_send(clients[i],NULL,NULL);
				}
			}
		}*/
		
	}
	return 0;
}
