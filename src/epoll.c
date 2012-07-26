#include "epoll.h"
#include "Socket.h"
#include "SocketWrapper.h"
#include "HandleMgr.h"
#include "SysTime.h"
#include <assert.h>
#include <stdio.h>

int32_t  epoll_init(engine_t e)
{
	assert(e);
	e->poller_fd = TEMP_FAILURE_RETRY(epoll_create(MAX_SOCKET));
    return 	e->poller_fd >= 0 ? 0:-1; 
}

int32_t epoll_register(engine_t e, socket_t s)
{
	assert(e);assert(s);
	int32_t ret = -1;	
	struct epoll_event ev;	
	ev.data.ptr = s;
	ev.events = EV_IN | EV_OUT | EV_ET;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_ADD,s->fd,&ev));
	return ret;
}


inline int32_t epoll_unregister(engine_t e,socket_t s)
{
	assert(e);assert(s);
	struct epoll_event ev;int32_t ret;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_DEL,s->fd,&ev));
	return ret;
}

int32_t epoll_loop(engine_t n,int32_t timeout)
{

	assert(n);	
	uint32_t ms;
	uint32_t tick = GetSystemMs();
	uint32_t _timeout = tick + timeout;
	do{
		while(!LINK_LIST_IS_EMPTY(n->actived))
		{
			socket_t s = LINK_LIST_POP(socket_t,n->actived);
			s->isactived = 0;
			if(Process(s) && s->isactived == 0)
			{
				s->isactived = 1;
				LINK_LIST_PUSH_BACK(n->actived,s);
			}
		}		
		ms = _timeout - tick;
		int32_t nfds = TEMP_FAILURE_RETRY(epoll_wait(n->poller_fd,n->events,MAX_SOCKET,ms));
		if(nfds < 0)
			return -1;
		int32_t i;
		for(i = 0 ; i < nfds ; ++i)
		{	
			socket_t sock = (socket_t)n->events[i].data.ptr;
			if(sock)
			{
				//套接口可读
				if(n->events[i].events & EPOLLIN)
				{	
					on_read_active(sock);
				}
				//套接口可写
				if(n->events[i].events & EPOLLOUT)
					on_write_active(sock);	
			
				if(n->events[i].events & EPOLLERR)
				{
					//套接口异常
				}
			}
		}	
		tick = GetSystemMs();
	}while(tick < _timeout);
	return 0;
}
