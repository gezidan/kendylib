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
typedef void (*item_destroyer)(void*);
void default_mq_item_destroyer(void* item);

#define MQ_DEFAULT_ITEM_DESTROYER default_mq_item_destroyer

mq_t create_mq(uint32_t,item_destroyer);
void destroy_mq(mq_t*);
void mq_push(mq_t,struct list_node*);
void mq_push_now(mq_t,struct list_node*);
void mq_push_list(mq_t,struct link_list *,uint32_t timeout);
struct list_node* mq_pop(mq_t,uint32_t timeout);

void  init_mq_system();
void  mq_flush();
#endif
