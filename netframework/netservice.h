/*	
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _NETSERVICE_H
#define _NETSERVICE_H

#include "mq.h"
#include "thread.h"
#include "KendyNet.h"
#include "Acceptor.h"
#include "msg.h"

struct netservice;
struct engine_struct
{
	mq_t     mq_in;
	HANDLE   engine;
	thread_t thread_engine;
	struct netservice *service;
};

#define MAX_ENGINES 64
typedef struct netservice
{
	struct engine_struct engines[MAX_ENGINES];
	uint32_t engine_count;
	mq_t     mq_out;
	thread_t thread_listen;
	acceptor_t _acceptor;
	volatile int8_t stop;
}*netservice_t;

//创建一个网络服务对象,thread_count表示要创建多少个线程,每个线程运行一个engine
netservice_t create_net_service(uint32_t thread_count);
void         stop_net_service(netservice_t);
void         destroy_net_service(netservice_t*);

//从网络服务队列中获取到来的消息
msg_t        net_peek_msg(netservice_t,uint32_t ms);

//添加一个网络监听
HANDLE       net_add_listener(netservice_t,const char *ip,uint32_t port);
//关闭一个网络监听
void         net_rem_listener(netservice_t,HANDLE);


#endif
