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
#include "common_define.h"
#include "db_protocal.h"


allocator_t wpacket_allocator = NULL;
uint32_t total_bytes_recv = 0;
atomic_32_t wpacket_count = 0;
atomic_32_t rpacket_count = 0; 
atomic_32_t buf_count = 0; 

global_table_t gtb; 

void on_client_disconnect(struct connection *c,int32_t reason)
{
	HANDLE sock = c->socket;
	if(0 == connection_destroy(&c))
	{
		ReleaseSocketWrapper(sock);
	}
}


void on_process_packet(struct connection *c,rpacket_t r)
{
	cache_protocal_t p;
	uint32_t coro_id = rpacket_read_uint32(r);
	uint8_t type = rpacket_read_uint8(r);
	switch(type)
	{
		case CACHE_GET:
			p = create_get();
			break;
		case CACHE_SET:
			p = create_set();
			break;
		case CACHE_DEL:
			p = create_del();
			break;			
	}
	wpacket_t ret = p->execute(gtb,r,coro_id);
	if(NULL != ret)
		connection_send(c,ret,NULL);
	destroy_protocal(&p);
	rpacket_destroy(&r);
}

void accept_callback(HANDLE s,void *ud)
{
	HANDLE *engine = (HANDLE*)ud;	
	struct connection *c = connection_create(s,0,SINGLE_THREAD,on_process_packet,on_client_disconnect);
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
	
	gtb = global_table_create(65536);
	
	int32_t i = 0;
	char key[64];
	for( ; i < 1000000; ++i)
	{
		basetype_t a = basetype_create_int32(i);
		snprintf(key,64,"test%d",i);
		a = global_table_insert(gtb,key,a,global_hash(key));
		if(!a)
			printf("error 1\n");
		basetype_release(&a);		
	}
			
	engine = CreateEngine();
	thread_run(_Listen,&engine);
	while(1)
	{
		EngineRun(engine,100);		
	}
	return 0;
}
