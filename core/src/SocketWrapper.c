#include "SocketWrapper.h"
#if defined(_LINUX)
#include "epoll.h"
#include "Socket.h"
#include "HandleMgr.h"

typedef int32_t SOCKET;

static void InitSocket(SOCK sock,SOCKET fd)
{
	socket_t s = GetSocketByHandle(sock);
	s->fd = fd;
	LINK_LIST_CLEAR(s->pending_send);
	LINK_LIST_CLEAR(s->pending_recv);
	s->readable = s->writeable = 0;
	s->engine = 0;
	s->isactived = 0;
	s->dnode.pre = s->dnode.next = NULL;
}

int32_t setNonblock(SOCK sock)
{

	socket_t s = GetSocketByHandle(sock);
	if(s)
	{
		int32_t fd_flags;
		int32_t nodelay = 1;

		if (setsockopt(s->fd, IPPROTO_TCP, TCP_NODELAY, (void *)&nodelay, sizeof(nodelay)))
			return -1;

		fd_flags = fcntl(s->fd, F_GETFL, 0);

#if defined(O_NONBLOCK)
		fd_flags |= O_NONBLOCK;
#elif defined(O_NDELAY)
		fd_flags |= O_NDELAY;
#elif defined(FNDELAY)
		fd_flags |= O_FNDELAY;
#else
		/* XXXX: this breaks things, but an alternative isn't obvious...*/
		return -1;
#endif

		if (fcntl(s->fd, F_SETFL, fd_flags) == -1) 
			return -1;

		return 0;
	}
	return -1;
}
#elif defined(_WIN)
#include "iocp.h"
#include "Socket.h"
#include "HandleMgr.h"
static void InitSocket(SOCK sock,SOCKET fd)
{
	socket_t s = GetSocketByHandle(sock);
	s->fd = fd;
	s->engine = NULL;
}

int32_t setNonblock(SOCK sock)
{

	socket_t s = GetSocketByHandle(sock);
	if(s)
	{
		uint32_t ul = 1;
		ioctlsocket(s->fd,FIONBIO,(uint32_t*)&ul);
		return 0;
	}
	return -1;
}
#endif


SOCK OpenSocket(int32_t family,int32_t type,int32_t protocol)
{
	SOCKET sockfd; 
	if( (sockfd = socket(family,type,protocol)) < 0)
	{
		return INVAILD_SOCK;
	}
	SOCK sock = NewSocketWrapper();
	if(sock < 0)
	{
		close(sockfd);
		return INVAILD_SOCK;
	}
	InitSocket(sock,sockfd);
	return sock;
}

int32_t CloseSocket(SOCK sock)
{
	socket_t s = GetSocketByHandle(sock);
	if(s)
	{
		return close(s->fd);
	}
	return -1;
}

void ReleaseSocket(SOCK sock)
{
	ReleaseSocketWrapper(sock);
}

int32_t Connect(SOCK sock,const struct sockaddr *servaddr,socklen_t addrlen)
{
	socket_t s = GetSocketByHandle(sock);
	if(s)
	{
		if(connect(s->fd,servaddr,addrlen) < 0)
		{
			//printf("%s\n",strerror(errno));
			return -1;
		}
		return 0;
	}
	return -1;

}

SOCK Tcp_Connect(const char *ip,uint16_t port,struct sockaddr_in *servaddr,int32_t retry)
{
	if(!ip)
		return INVAILD_SOCK;

	bzero(servaddr,sizeof(*servaddr));
	servaddr->sin_family = INET;
	servaddr->sin_port = htons(port);
	if(inet_pton(INET,ip,&servaddr->sin_addr) < 0)
	{

		printf("%s\n",strerror(errno));
		return INVAILD_SOCK;
	}
	
	SOCK sock = OpenSocket(INET,STREAM,TCP);
	if(sock)
	{
		while(1)
		{
			if(0 == Connect(sock,(struct sockaddr*)servaddr,sizeof(*servaddr)))
				return sock;
			if(!retry)
				break;
		}
		CloseSocket(sock);
	}
	return INVAILD_SOCK;
}

int32_t Bind(SOCK sock,const struct sockaddr *myaddr,socklen_t addrlen)
{
	socket_t s = GetSocketByHandle(sock);
	if(s)
	{
		if(bind(s->fd,myaddr,addrlen) < 0)
		{
			printf("%s\n",strerror(errno));
			return -1;
		}
		return 0;
	}
	return -1;
}

int32_t Listen(SOCK sock,int32_t backlog)
{
	socket_t s = GetSocketByHandle(sock);
	if(s)
	{
		if(listen(s->fd,backlog) < 0)
		{
			printf("%s\n",strerror(errno));
			return -1;
		}
		return 0;
	}
	return -1;
}

SOCK Tcp_Listen(const char *ip,uint16_t port,struct sockaddr_in *servaddr,int32_t backlog)
{
	SOCK sock;
	sock = OpenSocket(INET,STREAM,TCP);
	if(sock < 0)
		return INVAILD_SOCK;

	bzero(servaddr,sizeof(*servaddr));
	servaddr->sin_family = INET;
	if(ip)
	{
		if(inet_pton(INET,ip,&servaddr->sin_addr) < 0)
		{

			printf("%s\n",strerror(errno));
			return INVAILD_SOCK;
		}
	}
	else
		servaddr->sin_addr.s_addr = htonl(INADDR_ANY);	
	servaddr->sin_port = htons(port);

	if(Bind(sock,(struct sockaddr*)servaddr,sizeof(*servaddr)) < 0)
	{
		CloseSocket(sock);
		return INVAILD_SOCK;
	}

	if(Listen(sock,backlog) == 0) 
		return sock;
	else
	{
		CloseSocket(sock);
		return INVAILD_SOCK;
	}
}

SOCK Accept(SOCK sock,struct sockaddr *sa,socklen_t *salen)
{
	socket_t s = GetSocketByHandle(sock);
	if(s)
	{
		int32_t n;
	again:
		if((n = accept(s->fd,sa,salen)) < 0)
		{
#if defined(_LINUX)
	#ifdef EPROTO
			if(errno == EPROTO || errno == ECONNABORTED)
	#else
			if(errno == ECONNABORTED)
	#endif
				goto again;
			else
			{
				//printf("%s\n",strerror(errno));
				return INVAILD_SOCK;
			}
#elif defined(_WIN)
				return INVAILD_SOCK;
#endif
		}
		SOCK newsock = NewSocketWrapper();
		if(newsock < 0)
		{
			close(n);
			return INVAILD_SOCK;
		}
		InitSocket(newsock,n);		
		return newsock;
	}
	return INVAILD_SOCK;
}

int32_t getLocalAddrPort(SOCK sock,struct sockaddr_in *remoAddr,socklen_t *len,char *buf,uint16_t *port)
{

	socket_t s = GetSocketByHandle(sock);
	if(s)
	{
		if(0 == buf)
			return -1;
		int32_t ret = getsockname(s->fd, (struct sockaddr*)remoAddr,len);
		if(ret != 0)
			return -1;
		if(0 == inet_ntop(INET,&remoAddr->sin_addr,buf,15))
			return -1;
		*port = ntohs(remoAddr->sin_port);
		return 0;
	}
	return -1;
}


int32_t getRemoteAddrPort(SOCK sock,char *buf,uint16_t *port)
{
	socket_t s = GetSocketByHandle(sock);
	if(s)
	{
		if(0 == buf)
			return -1;
		struct sockaddr_in remoAddr;
		memset(&remoAddr,0,sizeof(remoAddr));
		remoAddr.sin_family = INET;
		socklen_t len = sizeof(remoAddr);
		int32_t ret = getpeername(s->fd,(struct sockaddr*)&remoAddr,&len);
		if(ret != 0)
			return -1;
		if(0 == inet_ntop(INET,&remoAddr.sin_addr,buf,15))
			return -1;
		*port = ntohs(remoAddr.sin_port);
		return 0;
	}
	return -1;
}
struct hostent *Gethostbyaddr(const char *ip,int32_t family)
{

	if(!ip)
		return NULL;
	struct sockaddr_in servaddr;
	struct hostent	*hptr;

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	if(inet_pton(family,ip,&servaddr.sin_addr) < 0)
	{
		return NULL;
	}
	if ( (hptr = gethostbyaddr(&servaddr.sin_addr,sizeof(servaddr.sin_addr),family)) == NULL) {
		return NULL;
	}

	return hptr;
}

