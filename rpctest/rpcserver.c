
#include "KendyNet.h"
#include "Connection.h"
#include <stdio.h>
#include <stdlib.h>
#include "thread.h"
#include "SocketWrapper.h"
#include "SysTime.h"
#include "Acceptor.h"
#include <stdint.h>
#include "block_obj_allocator.h"
#include <assert.h>
uint32_t packet_recv = 0;
uint32_t packet_send = 0;
uint32_t send_request = 0;
uint32_t tick = 0;
uint32_t now = 0;
uint32_t clientcount = 0;
uint32_t last_send_tick = 0;
allocator_t wpacket_allocator = NULL;
uint32_t total_bytes_recv = 0;

#define MAX_CLIENT 1000
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
	
	uint32_t coro_id = rpacket_read_uint32(r);
	const  char *function_name = rpacket_read_string(r);
	//printf("%d,%s\n",coro_id,function_name);
	uint32_t i = 0;
	wpacket_t w;
	for(; i < MAX_CLIENT; ++i)
	{
		if(clients[i])
		{
			//w = wpacket_create_by_rpacket(wpacket_allocator,r);
			w = wpacket_create(0,wpacket_allocator,64,0);
			wpacket_write_uint32(w,coro_id);
			if(strcmp(function_name,"remote_fun1") == 0)
				wpacket_write_uint32(w,1);
			else
				wpacket_write_uint32(w,2);
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
	send2_all_client(r);
	//++send_request;
	rpacket_destroy(&r);
	++packet_recv;	
}

void accept_callback(HANDLE s,void *ud)
{
	HANDLE *engine = (HANDLE*)ud;	
	struct connection *c = connection_create(s,0,0,on_process_packet,remove_client);
	add_client(c);
	printf("cli fd:%d\n",s);
	setNonblock(s);
	//·¢³öµÚÒ»¸ö¶ÁÇëÇó
	connection_start_recv(c);
	Bind2Engine(*engine,s,RecvFinish,SendFinish);
}


const char *ip;
uint32_t port;


void *_Listen(void *arg)
{
	struct listen_arg* args[2];
	args[0] = (struct listen_arg*)calloc(1,sizeof(*args[0]));
	args[0]->ip = ip;
	args[0]->port = port;
	args[0]->accept_callback = &accept_callback;
	args[0]->ud = arg;
	args[1] = NULL;
	acceptor_t a = create_acceptor((struct listen_arg**)&args);
	free(args[0]);
	while(1)
		acceptor_run(a,100);
	return 0;
}
uint32_t iocp_count = 0; 
int main(int argc,char **argv)
{

	HANDLE engine;
	uint32_t n;
	
	ip = argv[1];
	port = atoi(argv[2]);
	signal(SIGPIPE,SIG_IGN);
	if(InitNetSystem() != 0)
	{
		printf("Init error\n");
		return 0;
	}
	wpacket_allocator = (allocator_t)create_block_obj_allocator(0,sizeof(struct wpacket));	

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
		}
	}
	return 0;
}
