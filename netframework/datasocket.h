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

#include "Connection.h"
#include "refbase.h"
#include "mq.h"
#include "netservice.h"

typedef struct datasocket
{
	struct refbase _refbase;
	struct connection *c;
	mq_t           _mq;
	netservice_t   service;
	volatile int8_t is_close;
}*datasocket_t;

datasocket_t create_datasocket(netservice_t service,struct connection*,mq_t _mq);
void         close_datasocket(datasocket_t);    //active close datasocket connection
void         release_datasocket(datasocket_t*);  //decrease ref count
void	     acquire_datasocket(datasocket_t);  //increase ref count; 
int32_t      data_send(datasocket_t,wpacket_t);

#endif
