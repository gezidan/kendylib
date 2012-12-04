#ifndef _CORONET_H
#define _CORONET_H

#include "netservice.h"
#include "msg_loop.h"
#include "datasocket.h"
#include "SysTime.h"
#include "timing_wheel.h"

typedef struct coronet
{
	netservice_t    nets;
	msg_loop_t      msgl;
	sche_t          coro_sche;
	uint32_t        last_check_timer;
	TimingWheel_t   timer_ms;//精度50ms
	TimingWheel_t   timer_s; //精度1s
	TimingWheel_t   timer_m; //精度1分钟
}*coronet_t;


//执行完回调之后,如果需要重新加入到timer中返回1,否则返回0
typedef int32_t (coronet_timer_callback*)(void *ud,uint32_t now);

struct coronet_timer
{
	WheelItem_t wheel_item;
	coronet_timer_callback _callback;
	uint32_t timeout;
	coronet_t coron;
	void *ud;
};

coronet_t coronet_create();
void      coronet_init_net(coronet_t,on_packet,on_new_connection,on_connection_disconnect,on_send_block);
void      coronet_init_coro(int32_t max_coro,int32_t stack_size,void (*)(void*),void*);
void      coronet_run(coronet_t coron);

int32_t coronet_add_timer(coronet_t,coronet_timer_callback,void *ud,uint32_t timeout);


void coronet_destroy(coronet_t*);
void peek_msg(coronet_t,uint32_t);
void process_rpc_return(rpacket_t r);

//添加一个网络监听
HANDLE coronet_add_listener(coronet_t,const char *ip,uint32_t port);
//关闭一个网络监听
void coronet_rem_listener(coronet_t,HANDLE);

int32_t coronet_connect(coronet_t,const char *ip,uint32_t port);

#endif