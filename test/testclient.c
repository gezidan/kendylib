#include <stdio.h>
#include "core/SocketWrapper.h"
#include "core/SysTime.h"
#include "core/KendyNet.h"
#include "core/Connector.h"
#include "core/Connection.h"
#include "core/common_define.h"
#include "testnet.h"


connector_t con = NULL;
uint32_t iocp_count = 0;

void on_process_packet(struct connection *c,rpacket_t r)
{
	printf("on_process_packet\n");
	uint32_t s = rpacket_read_uint32(r);
	uint32_t t;
	if(s == (uint32_t)c->socket)
	{
		t = rpacket_read_uint32(r);
		uint32_t sys_t = GetSystemMs();
		ava_interval += (sys_t - t);
		ava_interval /= 2;
	}
	++packet_recv;
	rpacket_destroy(&r);
}

void on_connect_callback(SOCK s,const char *ip,int32_t port,void *ud)
{
	ENGINE *engine = (ENGINE*)ud;
	struct connection *c;
	wpacket_t wpk;
	++connect_count;
	if(s == NULL)
	{
		printf("connet to [%s:%d] failed\n",ip,port);
	}
	else
	{

		setNonblock(s);
		c = connection_create(s,0,SINGLE_THREAD,on_process_packet,remove_client);
		printf("connet to [%s:%d] success\n",ip,port);
		add_client(c);
		Bind2Engine(*engine,s,RecvFinish,SendFinish,NULL);
		wpk = wpacket_create(SINGLE_THREAD,NULL,64,0);
		wpacket_write_uint32(wpk,(uint32_t)s);
		uint32_t sys_t = GetSystemMs();
		wpacket_write_uint32(wpk,sys_t);
		wpacket_write_string(wpk,"hello kenny");
		connection_send(c,wpk,NULL);
		connection_start_recv(c);
	}
}

void send_finish(void *ptr,struct wpacket *wpk)
{
	struct connection *c = (struct connection *)ptr;
	connection_active_close(c);
}

void on_connect_callback2(SOCK s,const char *ip,int32_t port,void *ud)
{
	ENGINE *engine = (ENGINE*)ud;
	struct connection *c;
	wpacket_t wpk;
	++connect_count;
	if(s == NULL)
	{
		printf("connet to [%s:%d] failed\n",ip,port);
	}
	else
	{
		setNonblock(s);
		c = connection_create(s,0,SINGLE_THREAD,on_process_packet,remove_client);
		printf("connet to [%s:%d] success\n",ip,port);
		add_client(c);
		Bind2Engine(*engine,s,RecvFinish,SendFinish,NULL);
		wpk = wpacket_create(SINGLE_THREAD,NULL,64,0);
		wpacket_write_uint32(wpk,(uint32_t)s);
		uint32_t sys_t = GetSystemMs();
		wpacket_write_uint32(wpk,sys_t);
		wpacket_write_string(wpk,"hello kenny");
		connection_send(c,wpk,send_finish);//发送一个包，发送完成后关闭连接
		connection_start_recv(c);
	}
}

int32_t main(int32_t argc,char **argv)
{
	ENGINE engine;

	const char *ip = argv[1];
	uint32_t port = atoi(argv[2]);
	int32_t client_count = atoi(argv[3]);
	init_system_time(10);
	if(InitNetSystem() != 0)
	{
		printf("Init error\n");
		return 0;
	}
	wpacket_allocator = (allocator_t)create_block_obj_allocator(SINGLE_THREAD,sizeof(struct wpacket));

	int32_t ret;
	int32_t i = 0;
	uint32_t send_interval = 20;
	uint32_t send_tick = 0;
	wpacket_t wpk;

	init_clients();
	engine = CreateEngine();
	con =  connector_create();
	for( ; i < client_count;++i)
	{
		ret = connector_connect(con,ip,port,on_connect_callback2,&engine,1000*20);
		sleepms(1);
	}
	while(1)
	{
		connector_run(con,1);
		EngineRun(engine,20);
		/*now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("recv:%u,s_req:%u,ava_interval:%u\n",(packet_recv*1000)/(now-tick),send_request,ava_interval);
			tick = now;
			packet_recv = 0;
			send_request = 0;
			ava_interval = 0;
		}
		if(now - send_tick > send_interval)
		{
			send_tick = now;
			for(i = 0; i < client_count; ++i)
			{
					if(clients[i])
					{
						int j = 0;
						for( ; j < 1; ++j)
						{
						wpk = wpacket_create(SINGLE_THREAD,wpacket_allocator,64,0);
						wpacket_write_uint32(wpk,(uint32_t)clients[i]->socket);
						uint32_t sys_t = GetSystemMs();
						wpacket_write_uint32(wpk,sys_t);
						wpacket_write_string(wpk,"hello kenny");
						connection_send(clients[i],wpk,NULL);
						}
					}
			}

		}*/

	}
	CleanNetSystem();
	return 0;
}
