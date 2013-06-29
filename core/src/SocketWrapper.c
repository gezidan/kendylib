#include "SocketWrapper.h"
#include "common.h"
#include "Socket.h"
#include "HandleMgr.h"

#if defined(_LINUX)
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
		ioctlsocket(s->fd,FIONBIO,(u_long *)&ul);
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
		return INVALID_SOCK;
	}
	SOCK sock = NewSocketWrapper();
	if(sock < 0)
	{
		close(sockfd);
		return INVALID_SOCK;
	}
	InitSocket(sock,sockfd);
	return sock;
}

int32_t CloseSocket(SOCK sock)
{
	socket_t s = GetSocketByHandle(sock);
	if(s)
	{
#ifdef _LINUX
		return close(s->fd);
#else
		return closesocket(s->fd);
#endif
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
		return INVALID_SOCK;

	memset((void*)servaddr,0,sizeof(*servaddr));
	servaddr->sin_family = INET;
	servaddr->sin_port = htons(port);
#ifdef _WIN
	servaddr->sin_addr.s_addr = inet_addr(ip);
#else
	if(inet_pton(INET,ip,&servaddr->sin_addr) < 0)
	{

		printf("%s\n",strerror(errno));
		return INVALID_SOCK;
	}
#endif	
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
	return INVALID_SOCK;
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
		return INVALID_SOCK;

	memset((void*)servaddr,0,sizeof(*servaddr));
	servaddr->sin_family = INET;
	if(ip)
	{
#ifdef _WIN
		servaddr->sin_addr.s_addr = inet_addr(ip);
#else
		if(inet_pton(INET,ip,&servaddr->sin_addr) < 0)
		{

			printf("%s\n",strerror(errno));
			return INVALID_SOCK;
		}
#endif	
	}
	else
		servaddr->sin_addr.s_addr = htonl(INADDR_ANY);	
	servaddr->sin_port = htons(port);

	if(Bind(sock,(struct sockaddr*)servaddr,sizeof(*servaddr)) < 0)
	{
		CloseSocket(sock);
		return INVALID_SOCK;
	}

	if(Listen(sock,backlog) == 0) 
		return sock;
	else
	{
		CloseSocket(sock);
		return INVALID_SOCK;
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
				return INVALID_SOCK;
			}
#elif defined(_WIN)
				return INVALID_SOCK;
#endif
		}
		SOCK newsock = NewSocketWrapper();
		if(newsock < 0)
		{
			close(n);
			return INVALID_SOCK;
		}
		InitSocket(newsock,n);		
		return newsock;
	}
	return INVALID_SOCK;
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
#ifdef _WIN
		char *tmp = inet_ntoa(*(struct in_addr*)remoAddr);
		if(tmp)
			strcpy(buf,tmp);
		else
			return -1;
#else
		if(0 == inet_ntop(INET,&remoAddr->sin_addr,buf,15))
			return -1;
#endif
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
#ifdef _WIN
		char *tmp = inet_ntoa(*(struct in_addr*)&remoAddr);
		if(tmp)
			strcpy(buf,tmp);
		else
			return -1;
#else
		if(0 == inet_ntop(INET,&remoAddr.sin_addr,buf,15))
			return -1;
#endif
		*port = ntohs(remoAddr.sin_port);
		return 0;
	}
	return -1;
}

#ifdef _LINUX
ssize_t write_fd(int fd,void *ptr,size_t nbytes,int sendfd)
{
	struct msghdr msg;
	struct iovec iov[1];

	union
	{
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	}control_un;

	struct cmsghdr *cmptr;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);

	cmptr = CMSG_FIRSTHDR(&msg);
	cmptr->cmsg_len = CMSG_LEN(sizeof(int));
	cmptr->cmsg_level = SOL_SOCKET;
	cmptr->cmsg_type = SCM_RIGHTS;

	*((int*)CMSG_DATA(cmptr)) = sendfd;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov[0].iov_base = ptr;
	iov[0].iov_len = nbytes;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	return sendmsg(fd,&msg,0);
}

int create_un_execl(const char *path,const char *child)
{
	int fd, sockfd[2], status;
	pid_t		childpid;
	char		c, argsockfd[10], argmode[10];
	socketpair(LOCAL,STREAM, 0, sockfd);
	if ( (childpid = fork()) == 0) 
	{			
		close(sockfd[0]);		
		snprintf(argsockfd, sizeof(argsockfd), "%d", sockfd[1]);
		execl(path,child, argsockfd,(char *) NULL);
	}

	close(sockfd[1]);
	return sockfd[0];

}


ssize_t read_fd(int fd,void *ptr,size_t nbytes,int *recvfd)
{
	struct msghdr msg;
	struct iovec iov[1];
	ssize_t n;
	union
	{
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	}control_un;

	struct cmsghdr *cmptr;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);


	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov[0].iov_base = ptr;
	iov[0].iov_len = nbytes;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	if((n = recvmsg(fd,&msg,0)) <= 0)
	{
		return n;
	}


	if((cmptr = CMSG_FIRSTHDR(&msg)) != NULL && 
	   cmptr->cmsg_len == CMSG_LEN(sizeof(int)))
	{
		if(cmptr->cmsg_level != SOL_SOCKET)
		{
			exit(0);
		}
		if(cmptr->cmsg_type != SCM_RIGHTS)
		{
			exit(0);

		}
		*recvfd = *((int*)CMSG_DATA(cmptr));
	}
	else
		*recvfd = -1;

	return n;
}
#endif

/*
struct hostent *Gethostbyaddr(const char *ip,int32_t family)
{

	if(!ip)
		return NULL;
	struct sockaddr_in servaddr;
	struct hostent	*hptr;

	memset((void*)&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
#ifdef _WIN
	servaddr.sin_addr.s_addr = inet_addr(ip);
#else
	if(inet_pton(INET,ip,&servaddr.sin_addr) < 0)
	{

		printf("%s\n",strerror(errno));
		return NULL;
	}
#endif	

	if ( (hptr = gethostbyaddr((const char*)&servaddr.sin_addr,sizeof(servaddr.sin_addr),family)) == NULL) {
		return NULL;
	}

	return hptr;
}
*/
