//#include <winsock2.h>
//#include <WinBase.h>
//#include <Winerror.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include "SocketWrapper.h"
#include "HandleMgr.h" 
#include "SysTime.h"
#include "Acceptor.h"
#include "Socket.h"
#include "link_list.h"
#include "epoll.h"

struct st_listen
{
    list_node next;
   	on_accept accept_callback;
	HANDLE    sock;
	int32_t   real_fd;
	void *ud; 		
};

struct acceptor
{
	int32_t poller_fd;
	struct epoll_event events[1024];
	struct link_list *st_listens;
};

acceptor_t create_acceptor()
{
	acceptor_t a = (acceptor_t)calloc(1,sizeof(*a));
	a->poller_fd = TEMP_FAILURE_RETRY(epoll_create(1024));
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
		CloseSocket(tmp->sock);
		free(tmp);
	}
	LINK_LIST_DESTROY(&((*a)->st_listens));		
	free(*a);
	*a = NULL;
}

HANDLE    add_listener(acceptor_t a,const char *ip,uint32_t port,on_accept call_back,void *ud)
{
	if(!a)
		return -1;
	HANDLE ListenSocket;
	struct sockaddr_in servaddr;
	ListenSocket = Tcp_Listen(ip,port,&servaddr,256);
	if(ListenSocket >= 0)
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
			CloseSocket(ListenSocket);
			printf("listen %s:%d error\n",ip,port);
			return -1;
		}
		LINK_LIST_PUSH_BACK(a->st_listens,_st);
		return ListenSocket;
	}
	else
	{
		printf("listen %s:%d error\n",ip,port);
		return -1;
	}	
}

void rem_listener(acceptor_t a,HANDLE l)
{
	if(a)
	{
		struct epoll_event ev;int32_t ret;
		TEMP_FAILURE_RETRY(ret = epoll_ctl(a->poller_fd,EPOLL_CTL_DEL,GetSocketByHandle(l)->fd,&ev));
	}
}


void acceptor_run(acceptor_t a,int32_t timeout)
{
	uint32_t ms;
	uint32_t tick = GetSystemMs();
	uint32_t _timeout = tick + timeout;
	HANDLE client;
	struct sockaddr_in ClientAddress;
	int32_t nClientLength = sizeof(ClientAddress);
	do{	
		ms = _timeout - tick;
		int32_t nfds = TEMP_FAILURE_RETRY(epoll_wait(a->poller_fd,a->events,1024,ms));
		if(nfds == 0)
		{
			::sleepms(ms);
		}
		else
		{
			int32_t i;
			for(i = 0 ; i < nfds ; ++i)
			{	
				struct st_listen *_st = (struct st_listen *)a->events[i].data.ptr;
				if(_st)
				{
					//�׽ӿڿɶ�
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
