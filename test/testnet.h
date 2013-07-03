#include <assert.h>
#include "core/KendyNet.h"
#include "core/Connection.h"
#include <stdio.h>
#include <stdlib.h>
#include "core/thread.h"
#include "core/SocketWrapper.h"
#include "core/SysTime.h"
#include "core/Acceptor.h"
#include <stdint.h>
#include "core/block_obj_allocator.h"
#include "core/common_define.h"
#include "core/Connector.h"

uint32_t packet_recv = 0;
uint32_t packet_send = 0;
uint32_t send_request = 0;
uint32_t tick = 0;
uint32_t now = 0;
uint32_t clientcount = 0;
uint32_t last_send_tick = 0;
allocator_t wpacket_allocator = NULL;
uint32_t total_bytes_recv = 0;
static int32_t connect_count = 0;
uint32_t last_recv = 0;
uint32_t ava_interval = 0;

#define MAX_CLIENT 2000
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
	uint32_t i = 0;
	wpacket_t w;
	for(; i < MAX_CLIENT; ++i)
	{
		if(clients[i])
		{
			w = wpacket_create_by_rpacket(wpacket_allocator,r);
			assert(w);
			++send_request;
			connection_send(clients[i],w,NULL);
		}
	}
}

void remove_client(struct connection *c,uint32_t reason)
{
	printf("client disconnect,reason:%u\n",reason);
	uint32_t i = 0;
	for(; i < MAX_CLIENT; ++i)
	{
		if(clients[i] == c)
		{
			clients[i] = 0;
			break;
		}
	}
	SOCK sock = c->socket;
	if(0 == connection_destroy(&c))
	{
		ReleaseSocketWrapper(sock);
	}
}
