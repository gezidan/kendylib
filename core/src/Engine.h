#ifndef _ENGINE_H
#define _ENGINE_H

#include "sync.h"
#include "link_list.h"
#include "double_link.h"
#include <stdint.h>
#include "KendyNet.h"
struct socket_wrapper;

#if defined(_LINUX)

typedef struct engine
{
	int32_t  (*Init)(struct engine*);
	int32_t (*Loop)(struct engine*,int32_t timeout);
	int32_t  (*Register)(struct engine*,struct socket_wrapper*);
	int32_t  (*UnRegister)(struct engine*,struct socket_wrapper*);
	int32_t poller_fd;
	struct epoll_event events[MAX_SOCKET];
	struct double_link *actived;
}*engine_t;

#elif defined(_WIN)

typedef struct engine
{
	int32_t  (*Init)(struct engine*);
	int32_t (*Loop)(struct engine*,int32_t timeout);
	int32_t  (*Register)(struct engine*,struct socket_wrapper*);
	int32_t  (*UnRegister)(struct engine*,struct socket_wrapper*);
	HANDLE  complete_port;
}*engine_t;

#endif

engine_t create_engine();
void   free_engine(engine_t *);

#endif
