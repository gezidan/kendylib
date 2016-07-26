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
#include "KendyNet.h"

#if defined(_LINUX)
typedef struct socket_wrapper
{
	struct double_link_node dnode;
	struct engine  *engine;
	volatile int32_t isactived;//��ǰ�Ƿ���actived�б���
	volatile int32_t readable;
	volatile int32_t writeable;
	int32_t fd;
	struct link_list *pending_send;//��δ����ķ�����
	struct link_list *pending_recv;//��δ����Ķ�����
	void (*OnRead)(int32_t,st_io*,uint32_t err_code);
	void (*OnWrite)(int32_t,st_io*,uint32_t err_code);
	void (*OnClear_pending_io)(st_io*);
}*socket_t;


void on_read_active(socket_t);
void on_write_active(socket_t);
int32_t  Process(socket_t);

#elif defined(_WIN)

typedef struct socket_wrapper
{
	struct engine  *engine;
	SOCKET fd;
	struct link_list *pending_send;//��δ����ķ�����
	struct link_list *pending_recv;//��δ����Ķ�����	
	void (*OnRead)(int32_t,st_io*,uint32_t err_code);
	void (*OnWrite)(int32_t,st_io*,uint32_t err_code);
	void (*OnClear_pending_io)(st_io*);
}*socket_t;

#endif

socket_t create_socket();
void free_socket(socket_t*);
int32_t raw_send(socket_t s,st_io *io_req,uint32_t *err_code);
int32_t raw_recv(socket_t s,st_io *io_req,uint32_t *err_code);

#endif
