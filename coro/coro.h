#ifndef _CORO_H
#define _CORO_H
#include "uthread.h"
#include "minheap.h"
#include "link_list.h"

enum
{
	CORO_YIELD = 0,
	CORO_SLEEP,
	CORO_ACTIVE,
	CORO_DIE,
	CORO_START,
};

struct sche;
typedef struct coro
{
	struct list_node next;
	struct heapele _heapele;
	struct sche *_sche;
	uthread_t    ut;
	void *stack;
	uint8_t status;
	uint32_t timeout;
	void *arg;
	void* (*fun)(void *);	
}*coro_t;

coro_t coro_create(struct sche *,uint32_t stack_size,void*(*fun)(void*));
void coro_destroy(coro_t *);

extern inline coro_t get_current_coro();
void coro_sleep(coro_t,int32_t);
void coro_yield(coro_t);



#endif
