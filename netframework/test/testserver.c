#include "netservice.h"
#include "msg_loop.h"
#include "datasocket.h"
#include "SysTime.h"


#define MAX_CLIENT 1000
static datasocket_t clients[MAX_CLIENT];
uint32_t send_count = 0;
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
			w = wpacket_create_by_rpacket(NULL,r);
			data_send(clients[i],w);
			send_count++;
		}
	}
}

void send2_client(datasocket_t c,rpacket_t r)
{
	wpacket_t w = wpacket_create_by_rpacket(NULL,r);
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

const char *ip;
uint32_t port;
int main(int argc,char **argv)
{
	init_system_time(10);
	ip = argv[1];
	port = atoi(argv[2]);
	signal(SIGPIPE,SIG_IGN);
	init_mq_system();
	init_clients();
	if(InitNetSystem() != 0)
	{
		printf("Init error\n");
		return 0;
	}
	
	netservice_t n = create_net_service(1);
	net_add_listener(n,ip,port);
	msg_loop_t m = create_msg_loop(server_process_packet,process_new_connection,process_connection_disconnect);
	
	uint32_t now;
	uint32_t tick = GetSystemMs();
	while(1)
	{
		msg_loop_once(m,n,100);	
		now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("total send:%u,total_recv:%u\n",send_count,total_bytes_recv/1024/1024);
			tick = now;
			total_bytes_recv = 0;
			send_count = 0;
		}
	}

	return 0;
}

