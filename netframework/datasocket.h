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
#ifndef _DATASOCKET_H
#define _DATASOCKET_H

#include "net/Connection.h"
#include "util/refbase.h"
#include "util/mq.h"
#include "netservice.h"

struct engine_struct;
typedef uint64_t connd_t;
typedef struct datasocket
{
	struct refbase _refbase;
	connd_t c;
	//struct connection *c;
	mq_t           _mq;
	struct engine_struct *e;
	int32_t  close_reason;
	volatile int8_t is_close;
	uint32_t recv_timeout;
	uint32_t send_timeout;
}*datasocket_t;

datasocket_t create_datasocket(struct engine_struct *e,connd_t,mq_t _mq);
void         close_datasocket(datasocket_t);    //active close datasocket connection
void         release_datasocket(datasocket_t*);  //decrease ref count
void	     acquire_datasocket(datasocket_t);  //increase ref count; 
int32_t      data_send(datasocket_t,wpacket_t);
int32_t      set_recv_timeout(datasocket_t,uint32_t ms);
int32_t      set_send_timeout(datasocket_t,uint32_t ms);

#endif
