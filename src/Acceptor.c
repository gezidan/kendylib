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
	fd_set Set;
	struct link_list *st_listens;
};

acceptor_t create_acceptor(struct listen_arg **args)
{
	acceptor_t a = (acceptor_t)calloc(1,sizeof(*a));
	a->st_listens = LINK_LIST_CREATE();
	int i = 0;
	for(; args[i] != NULL; ++i)
	{
		HANDLE ListenSocket;
		struct sockaddr_in servaddr;
		ListenSocket = Tcp_Listen(args[i]->ip,args[i]->port,&servaddr,256);
		if(ListenSocket >= 0)
		{
			socket_t s = GetSocketByHandle(ListenSocket);
			setNonblock(ListenSocket);
			struct st_listen *_st = (struct st_listen *)calloc(1,sizeof(*_st));
			_st->accept_callback = args[i]->accept_callback;
			_st->ud = args[i]->ud;
			_st->sock = ListenSocket;
			_st->real_fd = GetSocketByHandle(ListenSocket)->fd;
			FD_SET(_st->real_fd,&a->Set);
			LINK_LIST_PUSH_BACK(a->st_listens,_st);
		}
		else
		{
			printf("listen %s:%d error\n",args[i]->ip,args[i]->port);
		}	
	}
	if(link_list_is_empty(a->st_listens))
	{
		LINK_LIST_DESTROY(&(a->st_listens));
		free(a);
		a = NULL;
	}
	return a;
}

void destroy_acceptor(acceptor_t *a)
{
	list_node *_st = link_list_head((*a)->st_listens);
	while(_st)
	{
		struct st_listen *tmp = (struct st_listen *)_st;
		CloseSocket(tmp->sock);
		_st = _st->next;
		free(tmp);
	}
	LINK_LIST_DESTROY(&((*a)->st_listens));	
	free(*a);
	*a = NULL;
}

void acceptor_run(acceptor_t a,int32_t ms)
{
	struct timeval timeout;
	HANDLE client;
	struct sockaddr_in ClientAddress;
	int32_t nClientLength = sizeof(ClientAddress);
	uint32_t tick,_timeout,_ms;
	tick = GetSystemMs();
	_timeout = tick + ms;
	list_node *_st = NULL;
	do
	{
		_ms = _timeout - tick;
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000*_ms;
		if(select(100, &a->Set,NULL, NULL, &timeout) >0 )
		{
			_st = link_list_head(a->st_listens);
			while(_st)
			{
				struct st_listen *tmp = (struct st_listen *)_st;	
				if(FD_ISSET(tmp->real_fd, &a->Set))
				{			
					for(;;)
					{
						client = Accept(tmp->sock, (struct sockaddr*)&ClientAddress, &nClientLength);
						if (client < 0)
							break;
						else
						{
							tmp->accept_callback(client,tmp->ud);
						}
					}
				}
				_st = _st->next;
			}
		}

		FD_ZERO(&a->Set);
		_st = link_list_head(a->st_listens);
		while(_st)
		{
			struct st_listen *tmp = (struct st_listen *)_st;
			FD_SET(tmp->real_fd,&a->Set);
			_st = _st->next;
		}
		tick = GetSystemMs();
	}while(tick < _timeout);

}
