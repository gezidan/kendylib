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
atomic_32_t wpacket_count = 0;
atomic_32_t rpacket_count = 0;
atomic_32_t buf_count = 0;
allocator_t wpacket_allocator = NULL;

void remove_client(struct connection *c,int32_t reason)
{	
	HANDLE sock = c->socket;
	if(0 == connection_destroy(&c))
	{
		ReleaseSocketWrapper(sock);
	}
}

void on_process_packet(struct connection *c,rpacket_t r)
{
	uint32_t coro_id = rpacket_read_uint32(r);
	const  char *function_name = rpacket_read_string(r);
	int32_t arg1 = rpacket_read_uint32(r);
	int32_t arg2 = rpacket_read_uint32(r);
	uint32_t i = 0;
	wpacket_t w = wpacket_create(SINGLE_THREAD,wpacket_allocator,64,0);
	wpacket_write_uint32(w,coro_id);
	if(strcmp(function_name,"sum") == 0)
		wpacket_write_uint32(w,arg1+arg2);
	else
		wpacket_write_uint32(w,arg1*arg2);
	assert(w);
	connection_send(c,w,NULL);
	rpacket_destroy(&r);
}

void accept_callback(HANDLE s,void *ud)
{
	HANDLE *engine = (HANDLE*)ud;	
	struct connection *c = connection_create(s,0,SINGLE_THREAD,on_process_packet,remove_client);
	printf("cli fd:%d\n",s);
	setNonblock(s);
	//·¢³öµÚÒ»¸ö¶ÁÇëÇó
	connection_start_recv(c);
	Bind2Engine(*engine,s,RecvFinish,SendFinish,NULL);
}


const char *ip;
uint32_t port;


void *_Listen(void *arg)
{
	/*struct listen_arg* args[2];
	args[0] = (struct listen_arg*)calloc(1,sizeof(*args[0]));
	args[0]->ip = ip;
	args[0]->port = port;
	args[0]->accept_callback = &accept_callback;
	args[0]->ud = arg;
	args[1] = NULL;*/
	acceptor_t a = create_acceptor();
	add_listener(a,ip,port,accept_callback,arg);
	//free(args[0]);
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
	init_system_time(10);
	if(InitNetSystem() != 0)
	{
		printf("Init error\n");
		return 0;
	}
	wpacket_allocator = (allocator_t)create_block_obj_allocator(SINGLE_THREAD,sizeof(struct wpacket));	
	engine = CreateEngine();
	thread_run(_Listen,&engine);
	while(1)
	{
		EngineRun(engine,100);
	}
	return 0;
}
