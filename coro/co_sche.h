#ifndef _COSCHE_H
#define _COSCHE_H

#include "minheap.h"
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

typedef struct sche
{
  	coro_t co;
  	int32_t max_coro;
  	int32_t stack_size;
  	minheap_t _minheap;
  	struct link_list *active_list;
  	int32_t next_check_timeout;
  	volatile int8_t  stop;
  	int32_t coro_size;
 	uint32_t ti;	 		
}*sche_t;


sche_t sche_create(int32_t max_coro,int32_t stack_size);
void sche_destroy(sche_t *);

void sche_schedule(sche_t );
struct coro *sche_spawn(sche_t,void*(*fun)(void*),void*arg);
coro_t coro_create(struct sche *,uint32_t stack_size,void*(*fun)(void*));
void coro_destroy(coro_t *);

extern inline coro_t get_current_coro();
extern inline void coro_sleep(coro_t,int32_t);
extern inline void coro_yield(coro_t);

#endif
