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

struct acceptor
{
	on_accept accept_callback;
	HANDLE    sock;
	int32_t real_fd;
	fd_set Set;
	void *ud;
};

acceptor_t create_acceptor(const char *ip,uint32_t port,on_accept accept_callback,void *ud)
{
	HANDLE ListenSocket;
	struct sockaddr_in servaddr;
	acceptor_t a;
	ListenSocket = Tcp_Listen(ip,port,&servaddr,5);
	socket_t s = GetSocketByHandle(ListenSocket);
	
	setNonblock(ListenSocket);
	
	a = malloc(sizeof(*a));
	a->sock = ListenSocket;
	a->accept_callback = accept_callback;
	a->ud = ud;
	a->real_fd = s->fd;
	FD_ZERO(&a->Set);
	FD_SET(a->real_fd,&a->Set);
	return a;
}

void destroy_acceptor(acceptor_t *a)
{
	CloseSocket((*a)->sock);
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
	do
	{
		_ms = _timeout - tick;
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000*_ms;
		if(select(100, &a->Set,NULL, NULL, &timeout) >0 )
		{
			for(;;)
			{
				client = Accept(a->sock, (struct sockaddr*)&ClientAddress, &nClientLength);
				if (client < 0)
					break;
				else
				{
					a->accept_callback(client,a->ud);
				}
			}
		}

		FD_ZERO(&a->Set);
		FD_SET(a->real_fd,&a->Set);
		tick = GetSystemMs();
	}while(tick < _timeout);

}