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
#ifndef _CONNECTOR_H
#define _CONNECTOR_H

#include <stdint.h>
typedef struct connector *connector_t;
typedef void (*on_connect)(SOCK,const char *ip,int32_t port,void*ud);

connector_t connector_create();
void        connector_destroy(connector_t*);
int32_t         connector_connect(connector_t,const char *ip,uint32_t port,on_connect,void *ud,uint32_t ms);
void        connector_run(connector_t,uint32_t ms);

#endif