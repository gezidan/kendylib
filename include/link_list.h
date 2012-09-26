#ifndef _LINK_LIST_H
#define _LINK_LIST_H


#include "sync.h"
#include "KendyNet.h"
#include <stdint.h>
//typedef struct list_node list_node;

struct link_list;
struct block_queue;


void link_list_push_back(struct link_list*,list_node*);

void link_list_push_front(struct link_list*,list_node*);

list_node* link_list_pop(struct link_list*);

inline list_node* link_list_head(struct link_list*);

inline int32_t link_list_is_empty(struct link_list*);

struct link_list *create_link_list();

void destroy_link_list(struct link_list**);

void link_list_clear(struct link_list*);

inline int32_t link_list_size(struct link_list*);

void    link_list_swap(struct link_list*to,struct link_list*from);

#define LINK_LIST_PUSH_FRONT(L,N) link_list_push_front(L,(list_node*)N)

#define LINK_LIST_PUSH_BACK(L,N) link_list_push_back(L,(list_node*)N)

#define LINK_LIST_POP(T,L) (T)link_list_pop(L)

#define LINK_LIST_IS_EMPTY(L) link_list_is_empty(L)

#define LINK_LIST_CREATE() create_link_list()

#define LINK_LIST_DESTROY(L) destroy_link_list(L)

#define LINK_LIST_CLEAR(L) link_list_clear(L)


struct block_queue* create_block_queue();
void destroy_block_queue(struct block_queue**);
void block_queue_push(struct block_queue*,list_node*);
enum
{
	POP_SUCCESS = 0,
	POP_TIMEOUT = 1,
	POP_FORCE_WAKE_UP = 2,
};
int32_t block_queue_pop(struct block_queue*,list_node**,int32_t timeout);
void block_queue_force_wakeup(struct block_queue*);
void block_queue_clear(struct block_queue*);

void block_queue_swap(struct block_queue*bq,struct link_list*from);


#define BLOCK_QUEUE_PUSH(L,N) block_queue_push(L,(list_node*)N)

#define BLOCK_QUEUE_POP(L,N,M) block_queue_pop(L,(list_node**)N,M)

#define BLOCK_QUEUE_CREATE() create_block_queue()

#define BLOCK_QUEUE_DESTROY(L) destroy_block_queue(L)

#define BLOCK_QUEUE_CLEAR(L)  block_queue_clear(L)

#define BLOCK_QUEUE_FORCE_WAKEUP(L) block_queue_force_wakeup(L)


#endif
