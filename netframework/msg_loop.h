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

#ifndef _MSG_LOOP_H
#define _MSG_LOOP_H

#include "datasocket.h"
#include "net/rpacket.h"
#include "util/thread.h"
#include "netservice.h"

//主消息循环，用于提取并处理从网络层过来的消息

typedef void (*on_packet)(datasocket_t,rpacket_t);//网络包回调
typedef void (*on_new_connection)(datasocket_t);  //处理新到达的连接
typedef void (*on_connection_disconnect)(datasocket_t,int32_t reason);//处理连接关闭
typedef void (*on_send_block)(datasocket_t);//发送阻塞

typedef struct msg_loop
{
	on_packet _on_packet;
	on_new_connection _on_new_connection;
	on_connection_disconnect _on_connection_disconnect;
	on_send_block _on_send_block;
	uint32_t last_sync_tick;
}*msg_loop_t;



msg_loop_t create_msg_loop(on_packet,on_new_connection,on_connection_disconnect,on_send_block);
void msg_loop_once(msg_loop_t,netservice_t,uint32_t ms);
void destroy_msg_loop(msg_loop_t*);
#endif
