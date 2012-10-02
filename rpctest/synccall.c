#include <stdio.h>
#include "SysTime.h"
#include "link_list.h"
#include "co_sche.h"
#include "thread.h"
#include "spinlock.h"
#include "wpacket.h"
#include "rpacket.h"
#include <stdlib.h>
#include "mq.h"
allocator_t wpacket_allocator = NULL;
sche_t g_sche = NULL;
thread_t client;
thread_t server;
uint32_t call_count = 0;
mq_t msgQ1;
mq_t msgQ2;


static inline rpacket_t peek_msg(mq_t msgQ,uint32_t timeout)
{
	rpacket_t msg = NULL;
	wpacket_t pk = (wpacket_t)mq_pop(msgQ,timeout);
	//BLOCK_QUEUE_POP(msgQ,&pk,timeout);
	if(pk)
	{
		msg = rpacket_create_by_wpacket(NULL,pk);
		wpacket_destroy(&pk);
	}
	return msg;		
}

static inline void push_msg(mq_t msgQ,wpacket_t w)
{
	//BLOCK_QUEUE_PUSH(msgQ,w);
	mq_push(msgQ,(struct list_node*)w);
}

static inline int sum(int32_t arg1,int32_t arg2)
{
	//printf("sum\n");
	coro_t co = get_current_coro();
	
	wpacket_t wpk = wpacket_create(1,wpacket_allocator,64,0);
	wpacket_write_uint32(wpk,(int32_t)co);
	wpacket_write_string(wpk,"sum");
	wpacket_write_uint32(wpk,arg1);
	wpacket_write_uint32(wpk,arg2);
	push_msg(msgQ1,wpk);
	//send and block until the response from rpc server
	coro_block(co);
	int ret = rpacket_read_uint32(co->rpc_response);
	rpacket_destroy(&co->rpc_response);
	return ret;
}

static inline int product(int32_t arg1,int32_t arg2)
{
	//printf("product\n");
	coro_t co = get_current_coro();
	wpacket_t wpk = wpacket_create(1,wpacket_allocator,64,0);
	wpacket_write_uint32(wpk,(int32_t)co);
	wpacket_write_string(wpk,"product");
	wpacket_write_uint32(wpk,arg1);
	wpacket_write_uint32(wpk,arg2);	
	push_msg(msgQ1,wpk);
	//send and block until the response from rpc server
	coro_block(co);
	int ret = rpacket_read_uint32(co->rpc_response);
	rpacket_destroy(&co->rpc_response);
	return ret;
}


void *test_coro_fun1(void *arg)
{
	coro_t co = get_current_coro();
	while(1)
	{
		int32_t arg1 = rand()%4096;
		int32_t arg2 = rand()%4096;
		if(arg1+arg2 != sum(arg1,arg2))
		{
			printf("rpc error\n");
			exit(0);
		}
		coro_block(co);		
		++call_count;
	}
}

void *test_coro_fun2(void *arg)
{
	coro_t co = get_current_coro();
	while(1)
	{
		int32_t arg1 = rand()%512;
		int32_t arg2 = rand()%512;
		if(arg1*arg2 != product(arg1,arg2))
		{
			printf("rpc error\n");
			exit(0);
		}
		++call_count;
	}
}


static inline void  sche_idel(void *arg)
{
	uint32_t ms = link_list_is_empty(g_sche->active_list) ? 100 : 0;
	rpacket_t rpk = peek_msg(msgQ2,ms);
	if(rpk)
	{
		coro_t co = (coro_t)rpacket_read_uint32(rpk);
		co->rpc_response = rpk;
		coro_wakeup(co);
	}
}

void *client_routine(void *arg)
{
	uint32_t tick = GetSystemMs();	
	while(1)
	{
		sche_idel(NULL);
		uint32_t now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("call_count:%u\n",call_count);
			tick = now;
			call_count = 0;
		}		
		sche_schedule(g_sche);	
	}
}

void *server_routine(void *arg)
{
	while(1)
	{
		rpacket_t rpk = peek_msg(msgQ1,50);
		if(rpk)
		{
			//printf("recv pk\n");
			uint32_t coro_id = rpacket_read_uint32(rpk);
			const  char *function_name = rpacket_read_string(rpk);
			int32_t arg1 = rpacket_read_uint32(rpk);
			int32_t arg2 = rpacket_read_uint32(rpk);
			wpacket_t w = wpacket_create(1,wpacket_allocator,64,0);
			wpacket_write_uint32(w,coro_id);
			if(strcmp(function_name,"sum") == 0)
				wpacket_write_uint32(w,arg1+arg2);
			else
				wpacket_write_uint32(w,arg1*arg2);
			push_msg(msgQ2,w);
			rpacket_destroy(&rpk);
		}
	}
}

int main()
{
	init_system_time(10);
	
	wpacket_allocator = (allocator_t)create_block_obj_allocator(1,sizeof(struct wpacket));

	msgQ1 = create_mq(4096);//BLOCK_QUEUE_CREATE();
	msgQ2 = create_mq(4096);//BLOCK_QUEUE_CREATE();
	g_sche = sche_create(250000,4096,sche_idel,NULL);
			
	int i = 0;
	for(; i < 25000; ++i)
	{		
		if(i%2 == 0)
			sche_spawn(g_sche,test_coro_fun1,NULL);
		else
			sche_spawn(g_sche,test_coro_fun2,NULL);
	}

	thread_run(server_routine,NULL);
	sleepms(1000);
	thread_run(client_routine,NULL);	
	getchar();
	return 0;
}
