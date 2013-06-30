#include "testnet.h"



void on_process_packet(struct connection *c,rpacket_t r)
{
	send2_all_client(r);
	//wpacket_t w = wpacket_create_by_rpacket(wpacket_allocator,r);
	//connection_send(c,w,NULL);
	//++send_request;

	total_bytes_recv += rpacket_len(r);
	rpacket_destroy(&r);
	++packet_recv;
}

void accept_callback(SOCK s,void *ud)
{
	ENGINE *engine = (ENGINE*)ud;
	struct connection *c = connection_create(s,0,SINGLE_THREAD,on_process_packet,remove_client);
	add_client(c);
	setNonblock(s);
	connection_start_recv(c);
	Bind2Engine(*engine,s,RecvFinish,SendFinish,NULL);
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

	ENGINE engine;
	uint32_t n;
	init_system_time(5);
	ip = argv[1];
	port = atoi(argv[2]);
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
		EngineRun(engine,50);
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
	CleanNetSystem();
	return 0;
}
