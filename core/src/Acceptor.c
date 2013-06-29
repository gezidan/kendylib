#include "SocketWrapper.h"
#include "HandleMgr.h" 
#include "SysTime.h"
#include "Acceptor.h"
#include "Socket.h"
#include "link_list.h"
#include "KendyNet.h"

#if defined(_LINUX)
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
struct st_listen
{
    list_node next;
   	on_accept accept_callback;
	SOCK    sock;
	int32_t   real_fd;
	void *ud; 		
};

#define MAX_LISTENER 1024

struct acceptor
{
	int32_t poller_fd;
	struct epoll_event events[MAX_LISTENER];
	struct link_list *st_listens;
};

acceptor_t create_acceptor()
{
	acceptor_t a = (acceptor_t)calloc(1,sizeof(*a));
	a->poller_fd = TEMP_FAILURE_RETRY(epoll_create(MAX_LISTENER));
	if(a->poller_fd < 0)
	{
		free(a);
		a = NULL;
	}
	a->st_listens = LINK_LIST_CREATE();
	return a;
}

void destroy_acceptor(acceptor_t *a)
{
	close((*a)->poller_fd);
	list_node *_st = link_list_head((*a)->st_listens);
	while(_st)
	{
		struct st_listen *tmp = (struct st_listen *)_st;
		_st = _st->next;
		ReleaseSocket(tmp->sock);
		free(tmp);
	}
	LINK_LIST_DESTROY(&((*a)->st_listens));		
	free(*a);
	*a = NULL;
}

SOCK    add_listener(acceptor_t a,const char *ip,uint32_t port,on_accept call_back,void *ud)
{
	if(!a)
		return INVALID_SOCK;
	SOCK ListenSocket;
	struct sockaddr_in servaddr;
	ListenSocket = Tcp_Listen(ip,port,&servaddr,256);
	if(ListenSocket != INVALID_SOCK)
	{
		struct st_listen *_st = (struct st_listen *)calloc(1,sizeof(*_st));
		_st->accept_callback = call_back;
		_st->ud = ud;
		_st->sock = ListenSocket;
		_st->real_fd = GetSocketByHandle(ListenSocket)->fd;
		
		int32_t ret = -1;	
		struct epoll_event ev;	
		ev.data.ptr = _st;
		ev.events = EV_IN;
		TEMP_FAILURE_RETRY(ret = epoll_ctl(a->poller_fd,EPOLL_CTL_ADD,_st->real_fd,&ev));
		if(ret == -1)
		{
			ReleaseSocket(ListenSocket);
			printf("listen %s:%d error\n",ip,port);
			return INVALID_SOCK;
		}
		LINK_LIST_PUSH_BACK(a->st_listens,_st);
		return ListenSocket;
	}
	else
	{
		ReleaseSocket(ListenSocket);
		printf("listen %s:%d error\n",ip,port);
		return INVALID_SOCK;
	}	
}

void rem_listener(acceptor_t a,SOCK l)
{
	if(a)
	{
		struct epoll_event ev;int32_t ret;
		TEMP_FAILURE_RETRY(ret = epoll_ctl(a->poller_fd,EPOLL_CTL_DEL,GetSocketByHandle(l)->fd,&ev));
		if(ret == 0)
		{
			int32_t count = link_list_size(a->st_listens);
			while(count>0)
			{
				struct st_listen *_st = LINK_LIST_POP(struct st_listen *,a->st_listens);
				if(_st->sock == l)
				{
					ReleaseSocket(l);
					free(_st);
					break;
				}
				else
					LINK_LIST_PUSH_BACK(a->st_listens,_st);
				--count;
			};
		}
	}
}


void acceptor_run(acceptor_t a,int32_t timeout)
{
	uint32_t ms;
	uint32_t tick = GetSystemMs();
	uint32_t _timeout = tick + timeout;
	SOCK client;
	struct sockaddr_in ClientAddress;
	int32_t nClientLength = sizeof(ClientAddress);
	do{	
		ms = _timeout - tick;
		int32_t nfds = TEMP_FAILURE_RETRY(epoll_wait(a->poller_fd,a->events,MAX_LISTENER,ms));
		if(nfds == 0)
		{
			sleepms(ms);
		}
		else
		{
			int32_t i;
			for(i = 0 ; i < nfds ; ++i)
			{	
				struct st_listen *_st = (struct st_listen *)a->events[i].data.ptr;
				if(_st)
				{
					//套接口可读
					if(a->events[i].events & EPOLLIN)
					{	
						client = Accept(_st->sock, (struct sockaddr*)&ClientAddress, (socklen_t*)&nClientLength);
						if (client < 0)
							break;
						else
						{
							_st->accept_callback(client,_st->ud);
						}
					}
				}
			}
		}		
		tick = GetSystemMs();
	}while(tick < _timeout);
}
#elif defined(_WIN)

struct st_listen
{
	list_node next;
	on_accept accept_callback;
	SOCK     sock;
	SOCKET   real_fd;
	void *ud; 		
};

struct acceptor
{
	FD_SET    Set;
	struct link_list *st_listens;
};

acceptor_t create_acceptor()
{
	acceptor_t a = (acceptor_t)calloc(1,sizeof(*a));
	a->st_listens = LINK_LIST_CREATE();
	return a;
}

SOCK    add_listener(acceptor_t a,const char *ip,uint32_t port,on_accept call_back,void *ud)
{
	if(!a)
		return INVALID_SOCK;
	SOCK ListenSocket;
	struct sockaddr_in servaddr;
	ListenSocket = Tcp_Listen(ip,port,&servaddr,256);
	if(ListenSocket != INVALID_SOCK)
	{
		struct st_listen *_st = (struct st_listen *)calloc(1,sizeof(*_st));
		_st->accept_callback = call_back;
		_st->ud = ud;
		_st->sock = ListenSocket;
		_st->real_fd = GetSocketByHandle(ListenSocket)->fd;
		LINK_LIST_PUSH_BACK(a->st_listens,_st);
	}
	else
		printf("创建监听失败\n");
	return ListenSocket;
}

void rem_listener(acceptor_t a,SOCK l)
{
	if(a)
	{
		int32_t count = link_list_size(a->st_listens);
		while(count>0)
		{
			struct st_listen *_st = LINK_LIST_POP(struct st_listen *,a->st_listens);
			if(_st->sock == l)
			{
				ReleaseSocket(l);
				free(_st);
				break;
			}
			else
				LINK_LIST_PUSH_BACK(a->st_listens,_st);
			--count;
		};
	}
}


void destroy_acceptor(acceptor_t *a)
{
	list_node *_st = link_list_head((*a)->st_listens);
	while(_st)
	{
		struct st_listen *tmp = (struct st_listen *)_st;
		_st = _st->next;
		ReleaseSocket(tmp->sock);
		free(tmp);
	}
	LINK_LIST_DESTROY(&((*a)->st_listens));		
	free(*a);
	*a = NULL;
}


void acceptor_run(acceptor_t a,int32_t ms)
{
	struct timeval timeout;
	SOCK client;
	struct sockaddr_in ClientAddress;
	int32_t nClientLength = sizeof(ClientAddress);
	DWORD tick,_timeout,_ms;
	tick = GetSystemMs();
	_timeout = tick + ms;
	do
	{
		FD_ZERO(&a->Set);
		list_node *_st = link_list_head(a->st_listens);
		while(_st)
		{
			struct st_listen *tmp = (struct st_listen *)_st;
			_st = _st->next;
			FD_SET(tmp->real_fd,&a->Set);
		}
		_ms = _timeout - tick;
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000*_ms;
		if(select(0, &a->Set,NULL, NULL, &timeout) >0 )
		{
			printf("一个套接口可读\n");
			list_node *_st = link_list_head(a->st_listens);
			while(_st)
			{
				struct st_listen *tmp = (struct st_listen *)_st;
				_st = _st->next;
				if(FD_ISSET(tmp->real_fd, &a->Set))
				{
					client = Accept(tmp->sock, (struct sockaddr*)&ClientAddress, &nClientLength);
					if (INVALID_SOCK == client)
						continue;
					else
					{
						tmp->accept_callback(client,tmp->ud);
					}
				}
			}
		}
		tick = GetSystemMs();
	}while(tick < _timeout);

}

#endif