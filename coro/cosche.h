#ifndef _COSCHE_H
#define _COSCHE_H

#include "minheap.h"
#include "uthread.h"
struct coro;
typedef struct sche
{
  	struct coro *co;
  	int32_t max_coro;
  	int32_t stack_size;
  	minheap_t _minheap;
  	struct link_list *active_list;
  	int32_t next_check_timeout;
  	volatile int8_t  stop;
  	int32_t coro_size;	
}*sche_t;


sche_t sche_create(int32_t max_coro,int32_t stack_size);
void sche_destroy(sche_t *);

void sche_schedule(sche_t );
struct coro *sche_spawn(sche_t,void*(*fun)(void*),void*arg);


#endif
