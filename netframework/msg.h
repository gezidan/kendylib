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

#ifndef _MSG_H
#define _MSG_H
#include "net/KendyNet.h"
#include "net/common_define.h"
typedef struct msg
{
	list_node next;
	uint8_t   type;
	uint64_t  usr_data;	
}*msg_t;


static inline msg_t create_msg(uint64_t usr_data,uint8_t type)
{
	if(type <= MSG_WPACKET || type >= MSG_END)
		return NULL;
	msg_t m = (msg_t)calloc(1,sizeof(*m));
	m->usr_data = usr_data;
	m->type = type;
	return m;	
}

static inline void  destroy_msg(msg_t *m)
{
	free(*m);
	*m = NULL;
}


#endif
