#ifdef _LINUX
#include "epoll.h"
#include "Socket.h"
#include "SocketWrapper.h"
#include "HandleMgr.h"
#include "SysTime.h"
#include "common.h"
#include <assert.h>


int32_t  epoll_init(engine_t e)
{
	assert(e);
	e->poller_fd = TEMP_FAILURE_RETRY(epoll_create(MAX_SOCKET));
	memset(e->events,0,sizeof(e->events));
	double_link_clear(e->actived);
	return 	e->poller_fd >= 0 ? 0:-1;
}

int32_t epoll_register(engine_t e, socket_t s)
{
	assert(e);assert(s);
	int32_t ret = -1;	
	struct epoll_event ev;	
	ev.data.ptr = s;
	ev.events = EV_IN | EV_OUT | EV_ET | EPOLLRDHUP;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_ADD,s->fd,&ev));
	return ret;
}


int32_t epoll_unregister(engine_t e,socket_t s)
{
	assert(e);assert(s);
	struct epoll_event ev;int32_t ret;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(e->poller_fd,EPOLL_CTL_DEL,s->fd,&ev));
	s->readable = s->writeable = 0;
	double_link_remove((struct double_link_node*)s);
	s->engine = NULL;
	return ret;
}

int32_t epoll_loop(engine_t n,int32_t ms)
{
	assert(n);	
	uint32_t sleep_ms;
	uint32_t timeout = GetSystemMs() + ms;
	uint32_t current_tick;
	uint32_t read_event = EV_IN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
	do{		
		while(!double_link_empty(n->actived))
		{
			socket_t s = (socket_t)double_link_pop(n->actived);
			s->isactived = 0;
			if(Process(s))
			{
				s->isactived = 1;
				double_link_push(n->actived,(struct double_link_node*)s);
			}
			if(GetSystemMs() >= timeout)
				break;
		}
		
		current_tick = GetSystemMs();
		sleep_ms = timeout > current_tick ? timeout - current_tick:0;	
		int32_t nfds = TEMP_FAILURE_RETRY(epoll_wait(n->poller_fd,n->events,MAX_SOCKET,sleep_ms));
		if(nfds < 0)
			return -1;
		int32_t i;
		for(i = 0 ; i < nfds ; ++i)
		{	
			socket_t sock = (socket_t)n->events[i].data.ptr;
			if(sock)
			{
				if(n->events[i].events & read_event)
					on_read_active(sock);
				if(n->events[i].events & EPOLLOUT)
					on_write_active(sock);	
			}
		}	
		current_tick = GetSystemMs();
	}while(timeout > current_tick);
	return 0;
}
#endif