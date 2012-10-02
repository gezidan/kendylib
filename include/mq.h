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
#ifndef _MQ_H
#define _MQ_H

#include <stdint.h>
#include "link_list.h"

typedef struct mq *mq_t;

mq_t create_mq(uint32_t);
void destroy_mq(mq_t*);
void mq_push(mq_t,struct list_node*);
struct list_node* mq_pop(mq_t,uint32_t timeout);
void mq_timeout(mq_t);

#endif
