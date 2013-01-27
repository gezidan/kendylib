#include "netservice.h"
#include "msg_loop.h"
#include "datasocket.h"
#include "util/SysTime.h"

atomic_32_t wpacket_count = 0;
atomic_32_t rpacket_count = 0;
atomic_32_t buf_count = 0;

#define MAX_CLIENT 5000
static datasocket_t clients[MAX_CLIENT];
uint32_t send_count = 0;
int32_t isusecount = 0;
void init_clients()
{
	uint32_t i = 0;
	for(; i < MAX_CLIENT;++i)
		clients[i] = NULL;
}

void add_client(datasocket_t c)
{
	uint32_t i = 0;
	for(; i < MAX_CLIENT; ++i)
	{
		if(clients[i] == NULL)
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
			w = get_wpacket_by_rpacket(r);
			data_send(clients[i],w);
			send_count++;
		}
	}
}

void send2_client(datasocket_t c,rpacket_t r)
{
	wpacket_t w = get_wpacket_by_rpacket(r);
	data_send(c,w);
}

void remove_client(datasocket_t c)
{
	uint32_t i = 0;
	for(; i < MAX_CLIENT; ++i)
	{
		if(clients[i] == c)
		{
			clients[i] = NULL;
			break;
		}
	}	
}

int32_t count = 0;

uint32_t total_bytes_recv = 0;


void server_process_packet(datasocket_t s,rpacket_t r)
{
	total_bytes_recv += rpacket_len(r);
	send2_all_client(r);
	//send2_client(s,r);	
}

void process_new_connection(datasocket_t s)
{
	set_recv_timeout(s,10*1000);
	set_send_timeout(s,10*1000);
	add_client(s);
	++count;
	printf("%d\n",count);
}

void process_connection_disconnect(datasocket_t s,int32_t reason)
{
	remove_client(s);
	release_datasocket(&s);
	--count;
	printf("%d\n",count);	
}

void process_send_block(datasocket_t s)
{
	remove_client(s);
	--count;
	printf("%d\n",count);		
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
	uint32_t thread_count = atoi(argv[3]);
	init_clients();	
	netservice_t n = create_net_service(thread_count);
	net_add_listener(n,ip,port);
	msg_loop_t m = create_msg_loop(server_process_packet,process_new_connection,process_connection_disconnect,process_send_block);
	uint32_t now;
	uint32_t tick = GetSystemMs();
	while(1)
	{
		msg_loop_once(m,n,100);	
		now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("total send:%u,total_recv:%u\n",isusecount,total_bytes_recv/1024/1024);
			//printf("w:%d,r:%d,b:%d\n",wpacket_count,rpacket_count,buf_count);
			tick = now;
			total_bytes_recv = 0;
			send_count = 0;
		}
	}

	return 0;
}

