
#include "db_protocal.h"
#include "dbtype.h"

#include <stdio.h>
#include "SocketWrapper.h"
#include "SysTime.h"
#include "KendyNet.h"
#include "Connector.h"
#include "Connection.h"
#include "common_define.h"
allocator_t wpacket_allocator = NULL;

atomic_32_t wpacket_count = 0;
atomic_32_t rpacket_count = 0; 
atomic_32_t buf_count = 0;  


connector_t con = NULL;

int8_t stage = 0;

int32_t setcount = 0;
int32_t getcount = 0;

uint32_t select_tick = 0;

void on_process_packet(struct connection *c,rpacket_t r)
{
	int8_t ret = rpacket_read_uint8(r);
	if(ret != 0)
	{
		printf("db op error:%d\n",stage);
		return;
	}
	
	if(stage == 0)
	{
		setcount++;
		if(setcount == 1000000)
		{
			printf("insert finish begin select----------\n");
			getcount = 0;
			select_tick = GetSystemMs();
			//发出100W条查询
			char key[64];
			int32_t i = 0;
			for( ; i < 1000000; ++i)
			{
				snprintf(key,64,"test%d",i);		
				wpacket_t wpk = wpacket_create(SINGLE_THREAD,NULL,64,0);
				wpacket_write_uint8(wpk,CACHE_GET);//设置
				wpacket_write_string(wpk,key);
				connection_send(c,wpk,NULL);	
			}			
			stage++;
		}
	}
	else
	{
		getcount++;
		if(getcount == 1000000)
		{
			select_tick = GetSystemMs() - select_tick;
			printf("select 100W finish:%u\n",select_tick);
			exit(0);
		}
	}
	
}

void on_connect_callback(HANDLE s,const char *ip,int32_t port,void *ud)
{
	HANDLE *engine = (HANDLE*)ud;
	struct connection *c;
	wpacket_t wpk;
	if(s == -1)
	{
		printf("%d,连接到:%s,%d,失败\n",s,ip,port);
	}
	else
	{
		
		setNonblock(s);
		c = connection_create(s,0,SINGLE_THREAD,on_process_packet,NULL);
		printf("%d,连接到:%s,%d,成功\n",s,ip,port);
		Bind2Engine(*engine,s,RecvFinish,SendFinish);
		//插入100W条数据
		char key[64];
		int32_t i = 0;
		for( ; i < 1000000; ++i)
		{
			snprintf(key,64,"test%d",i);		
			wpacket_t wpk = wpacket_create(SINGLE_THREAD,NULL,64,0);
			wpacket_write_uint8(wpk,CACHE_SET);//设置
			wpacket_write_string(wpk,key);
			wpacket_write_uint8(wpk,DB_INT32);//数据类型
			wpacket_write_uint32(wpk,i);
			connection_send(c,wpk,NULL);	
		}
		connection_start_recv(c);
	}
}

int32_t main(int32_t argc,char **argv)
{	
	HANDLE engine;
	const char *ip = argv[1];
	uint32_t port = atoi(argv[2]);
	signal(SIGPIPE,SIG_IGN);
	init_system_time(10);
	if(InitNetSystem() != 0)
	{
		printf("Init error\n");
		return 0;
	}	
	int32_t ret;
	int32_t i = 0;
	wpacket_t wpk;

	engine = CreateEngine();
	con =  connector_create();
	for( ; i < 1;++i)
	{
		ret = connector_connect(con,ip,port,on_connect_callback,&engine,1000*20);
		sleepms(1);
	}
	while(1)
	{
		connector_run(con,1);
		EngineRun(engine,1);
	}
	return 0;
}

