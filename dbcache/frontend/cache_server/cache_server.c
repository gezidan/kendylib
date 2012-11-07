#include "netservice.h"
#include "msg_loop.h"
#include "datasocket.h"
#include "SysTime.h"
#include "db_protocal.h"

atomic_32_t wpacket_count = 0;
atomic_32_t rpacket_count = 0;
atomic_32_t buf_count = 0;

global_table_t gtb;
void server_process_packet(datasocket_t s,rpacket_t r)
{
	//执行操作并返回结果
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
		data_send(s,ret);
	destroy_protocal(&p);
}

void process_new_connection(datasocket_t s)
{
	printf("w:%u,r:%u,b:%u\n",wpacket_count,rpacket_count,buf_count);
}

void process_connection_disconnect(datasocket_t s,int32_t reason)
{
	release_datasocket(&s);
	printf("w:%u,r:%u,b:%u\n",wpacket_count,rpacket_count,buf_count);
}

void process_send_block(datasocket_t s)
{
	//发送阻塞,直接关闭
	close_datasocket(s);
}


const char *ip;
uint32_t port;
int main(int argc,char **argv)
{
	init_net_service();
	ip = argv[1];
	port = atoi(argv[2]);
	netservice_t n = create_net_service(1);
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
	
	net_add_listener(n,ip,port);
	msg_loop_t m = create_msg_loop(server_process_packet,process_new_connection,process_connection_disconnect,process_send_block);
	while(1)
	{
		msg_loop_once(m,n,100);	
	}

	return 0;
}


