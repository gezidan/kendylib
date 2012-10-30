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


#endif
