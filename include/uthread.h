#ifndef _UTHREAD_H
#define _UTHREAD_H
#include <stdint.h>
typedef struct uthread *uthread_t;

uthread_t uthread_create(uthread_t parent,void*stack,uint32_t stack_size,void*(*fun)(void*));
void uthread_destroy(uthread_t);

extern void*uthread_switch(uthread_t from,uthread_t to,void *para);



#endif
