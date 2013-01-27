#include "db_protocal.h"
#include "dbtype.h"
#include <stdio.h>
#include "net/SocketWrapper.h"
#include "util/SysTime.h"
#include "net/KendyNet.h"
#include "net/Connector.h"
#include "net/Connection.h"
#include "net/common_define.h"
#include "netservice.h"
#include "msg_loop.h"
#include "co_sche.h"

sche_t g_sche = NULL;
uint32_t call_count = 0;

atomic_32_t wpacket_count = 0;
atomic_32_t rpacket_count = 0;
atomic_32_t buf_count = 0;
datasocket_t db_s;

int8_t test_select(const char *key,int32_t i)
{
	coro_t co = get_current_coro();
	wpacket_t wpk = get_wpacket(64);
	wpacket_write_uint32(wpk,(int32_t)co);
	wpacket_write_uint8(wpk,CACHE_GET);//ÉèÖÃ
	wpacket_write_string(wpk,key);
	data_send(db_s,wpk);
	coro_block(co);
	int8_t ret = rpacket_read_uint8(co->rpc_response);
	rpacket_read_uint8(co->rpc_response);
	int32_t val = rpacket_read_uint32(co->rpc_response);
	if(val != i)
		printf("error\n");
	//printf("begin\n");
	rpacket_destroy(&co->rpc_response);
	//printf("end\n");
	return ret;
}

void *test_coro_fun2(void *arg)
{
	coro_t co = get_current_coro();
	while(1)
	{
		char key[64];
		int32_t i = rand()%1000000;
		snprintf(key,64,"test%d",100);		
		if(0 == test_select(key,100))
			++call_count;
	}
}


void server_process_packet(datasocket_t s,rpacket_t r)
{
	coro_t co = (coro_t)rpacket_read_uint32(r);
	co->rpc_response = rpacket_create_by_rpacket(r);
	coro_wakeup(co);
}

void process_new_connection(datasocket_t s)
{
	printf("connect server\n");
	db_s = s;
	g_sche = sche_create(20000,65536,NULL,NULL);
	int i = 0;
	for(; i < 20000; ++i)
	{
		sche_spawn(g_sche,test_coro_fun2,NULL);
	}
}

void process_connection_disconnect(datasocket_t s,int32_t reason)
{
	release_datasocket(&s);
}

void process_send_block(datasocket_t s)
{
	//·¢ËÍ×èÈû,Ö±½Ó¹Ø±Õ
	close_datasocket(s);
}

int main(int argc,char **argv)
{
	init_net_service();
	const char *ip = argv[1];
	uint32_t port = atoi(argv[2]);
	netservice_t n = create_net_service(1);
	net_connect(n,ip,port);
	msg_loop_t m = create_msg_loop(server_process_packet,process_new_connection,process_connection_disconnect,process_send_block);
	uint32_t tick = GetSystemMs();
	while(1)
	{
		msg_loop_once(m,n,1);
		uint32_t now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("call_count:%u\n",(call_count*1000)/(now-tick));
			tick = now;
			call_count = 0;
		}
		if(g_sche)
			sche_schedule(g_sche);			
	}
	return 0;
}
