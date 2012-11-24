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
#ifndef _SOCKETWRAPPER_H
#define _SOCKETWRAPPER_H
#include <stdint.h>
#include "double_link.h"
typedef struct socket_wrapper
{
	struct double_link_node dnode;
	volatile int32_t status;//0:未开启;1:正常;
	struct engine  *engine;
	volatile int32_t isactived;//当前是否处于actived列表中	
	volatile int32_t readable;
	volatile int32_t writeable;
	int32_t fd;
	HANDLE handle;
	struct link_list *pending_send;//尚未处理的发请求
	struct link_list *pending_recv;//尚未处理的读请求
	void (*OnRead)(int32_t,st_io*);
	void (*OnWrite)(int32_t,st_io*);	
}*socket_t;


void on_read_active(socket_t);
void on_write_active(socket_t);
int32_t  Process(socket_t);
socket_t create_socket();
void free_socket(socket_t*);

int32_t raw_send(socket_t s,st_io *io_req,int32_t *bytes_transfer,uint32_t *err_code);
int32_t raw_recv(socket_t s,st_io *io_req,int32_t *bytes_transfer,uint32_t *err_code);

#endif
