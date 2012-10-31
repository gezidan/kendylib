#include "SocketWrapper.h"
#include "epoll.h"
#include "Socket.h"
#include "HandleMgr.h"

static void InitSocket(HANDLE sock,int32_t fd)
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

HANDLE OpenSocket(int32_t family,int32_t type,int32_t protocol)
{
	int32_t sockfd; 
	if( (sockfd = socket(family,type,protocol)) < 0)
	{
		return -1;
	}
	HANDLE sock = NewSocketWrapper();
	if(sock < 0)
	{
		close(sockfd);
		return -1;
	}
	InitSocket(sock,sockfd);
	return sock;
}

int32_t CloseSocket(HANDLE sock)
{
	socket_t s = GetSocketByHandle(sock);
	if(s)
	{
		return close(s->fd);
	}
	return -1;
}

int32_t ReleaseSocket(HANDLE sock)
{
	return ReleaseSocketWrapper(sock);
}

int32_t Connect(HANDLE sock,const struct sockaddr *servaddr,socklen_t addrlen)
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

int32_t Tcp_Connect(const char *ip,uint16_t port,struct sockaddr_in *servaddr,int32_t retry)
{
	if(!ip)
		return -1;

	bzero(servaddr,sizeof(*servaddr));
	servaddr->sin_family = INET;
	servaddr->sin_port = htons(port);
	if(inet_pton(INET,ip,&servaddr->sin_addr) < 0)
	{

		printf("%s\n",strerror(errno));
		return -1;
	}
	
	HANDLE sock = OpenSocket(INET,STREAM,TCP);
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
	return -1;
}

int32_t Bind(HANDLE sock,const struct sockaddr *myaddr,socklen_t addrlen)
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

int32_t Listen(HANDLE sock,int32_t backlog)
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

HANDLE Tcp_Listen(const char *ip,uint16_t port,struct sockaddr_in *servaddr,int32_t backlog)
{
	HANDLE sock;
	sock = OpenSocket(INET,STREAM,TCP);
	if(sock < 0)
		return -1;

	bzero(servaddr,sizeof(*servaddr));
	servaddr->sin_family = INET;
	if(ip)
	{
		if(inet_pton(INET,ip,&servaddr->sin_addr) < 0)
		{

			printf("%s\n",strerror(errno));
			return -1;
		}
	}
	else
		servaddr->sin_addr.s_addr = htonl(INADDR_ANY);	
	servaddr->sin_port = htons(port);

	if(Bind(sock,(struct sockaddr*)servaddr,sizeof(*servaddr)) < 0)
	{
		CloseSocket(sock);
		return -1;
	}

	if(Listen(sock,backlog) == 0) 
		return sock;
	else
	{
		CloseSocket(sock);
		return -1;
	}
}

HANDLE Accept(HANDLE sock,struct sockaddr *sa,socklen_t *salen)
{
	socket_t s = GetSocketByHandle(sock);
	if(s)
	{
		int32_t n;
	again:
		if((n = accept(s->fd,sa,salen)) < 0)
		{
	#ifdef EPROTO
			if(errno == EPROTO || errno == ECONNABORTED)
	#else
			if(errno == ECONNABORTED)
	#endif
				goto again;
			else
			{
				//printf("%s\n",strerror(errno));
				return -1;
			}
		}
		HANDLE newsock = NewSocketWrapper();
		if(newsock < 0)
		{
			close(n);
			return -1;
		}
		InitSocket(newsock,n);		
		return newsock;
	}
	return -1;
}

int32_t getLocalAddrPort(HANDLE sock,struct sockaddr_in *remoAddr,socklen_t *len,char *buf,uint16_t *port)
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


int32_t getRemoteAddrPort(HANDLE sock,char *buf,uint16_t *port)
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

/*
 * brief: 将文件描述符传递出去
 *
 * /
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

/ * 
 *  创建unix domain socket并创建子进程
 * /
int create_un_execl(const char *path,const char *child)
{
	/*建立unix domain socket,创建子进程执行data_worker* /
	int fd, sockfd[2], status;
	pid_t		childpid;
	char		c, argsockfd[10], argmode[10];
	socketpair(LOCAL,STREAM, 0, sockfd);
	if ( (childpid = fork()) == 0) 
	{		
		/* child process * /		
		close(sockfd[0]);		
		snprintf(argsockfd, sizeof(argsockfd), "%d", sockfd[1]);
		execl(path,child, argsockfd,(char *) NULL);
	}

	close(sockfd[1]);
	return sockfd[0];

}

/*
 * brief: 读取传递过来的描述符号
 * 
 * /
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
*/
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

int32_t setNonblock(HANDLE sock)
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

