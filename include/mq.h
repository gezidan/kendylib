#ifndef _MQ_H
#define _MQ_H

#include <stdint.h>
#include "link_list.h"

typedef struct mq *mq_t;

mq_t create_mq(uint32_t);
void destroy_mq(mq_t*);
void mq_push(mq_t,struct list_node*);
struct list_node* mq_pop(mq_t);
void mq_timeout(mq_t);

#endif
