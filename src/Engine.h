#ifndef _ENGINE_H
#define _ENGINE_H

#include "sync.h"
#include "link_list.h"
#include "double_link.h"
#include <stdint.h>
struct socket_wrapper;
typedef struct engine
{
	int32_t  (*Init)(struct engine*);
	int32_t (*Loop)(struct engine*,int32_t timeout);
	int32_t  (*Register)(struct engine*,struct socket_wrapper*);
	int32_t  (*UnRegister)(struct engine*,struct socket_wrapper*);
	volatile int32_t status; /*0:关闭状态,1:开启状态*/
	int32_t poller_fd;
	struct epoll_event events[MAX_SOCKET];
	struct double_link *actived;
	//struct link_list *actived;//激活且有数据需要操作的套接口
}*engine_t;

engine_t create_engine();
void   free_engine(engine_t *);

#endif
