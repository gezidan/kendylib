/*	
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/	

/*
# Text File
# Author:   kenny<huangweilook@21cn.com>
# File:     SocketWrapper.h 
# Created:  13:11:56 2008-10-27
# Modified: 13:12:06 2008-10-27
# Brief:    一些套接口API的包装函数.     
*/

#ifndef _SOCK_WRAPPER_H
#define _SOCK_WRAPPER_H

#include	<sys/types.h>	/* basic system data types */
#include	<sys/socket.h>	/* basic socket definitions */
#include	<sys/time.h>	/* timeval{} for select() */
#include	<time.h>		/* timespec{} for pselect() */
#include	<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<fcntl.h>		/* for nonblocking */
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	/* for S_xxx file mode constants */
#include	<sys/uio.h>		/* for iovec{} and readv/writev */
#include	<unistd.h>
#include	<sys/wait.h>
#include	<sys/un.h>		/* for Unix domain sockets */
#include    <net/if.h>
#include    <sys/ioctl.h>
#include    <netinet/tcp.h>
#include    <fcntl.h>
#include    <stdint.h>

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)\
   ({ long int __result;\
       do __result = (long int)(expression);\
       while(__result == -1L&& errno == EINTR);\
       __result;})
#endif

enum sock_family
{
	INET = AF_INET,
	INET6 = AF_INET6,
	LOCAL = AF_LOCAL,
	ROUTE = AF_ROUTE,
#ifdef _LINUX
	KEY = AF_KEY,
#endif
};

enum sock_type
{
	STREAM = SOCK_STREAM,//流协议
	DGRAM  = SOCK_STREAM,//数据报协议
	SEQPACKET = SOCK_SEQPACKET,
	RAW = SOCK_RAW,//原始套接口
};

enum sock_protocol
{
	TCP = IPPROTO_TCP,
	UDP = IPPROTO_UDP,
	SCTP = IPPROTO_SCTP,
};

//typedef sock_wrapper *socket_t;
#include "KendyNet.h"
HANDLE  OpenSocket(int32_t family,int32_t type,int32_t protocol);

int32_t  CloseSocket(HANDLE);

int32_t Connect(HANDLE sock,const struct sockaddr *servaddr,socklen_t addrlen);

/*
 * brief: 创建套接字,并与给定的对端建立连接.
 * para:  
 *        ip:对端的IP地址
 *        port:对端端口
 *        servaddr:
 *        retry: 如果connect失败是否重新尝试,直到连接成功才返回 
 * return: >0 套接字,-1,失败.
 */
HANDLE Tcp_Connect(const char *ip,uint16_t port,struct sockaddr_in *servaddr,int32_t retry);

int32_t Bind(HANDLE sock,const struct sockaddr *myaddr,socklen_t addrlen);

int32_t Listen(HANDLE sock,int32_t backlog);

/*
 * brief: 创建套接字,磅定,然后在此套接字上监听.
 *
 */
HANDLE Tcp_Listen(const char *ip,uint16_t port,struct sockaddr_in *servaddr,int32_t backlog);


HANDLE Accept(HANDLE,struct sockaddr *sa,socklen_t *salen);

/*
 * brief: 获取远端连接的IP,端口号.
 */
int32_t getRemoteAddrPort(HANDLE sock,char *buf,uint16_t *port);
int32_t getLocalAddrPort(HANDLE sock,struct sockaddr_in *remoAddr,socklen_t *len,char *buf,uint16_t *port);

/*
ssize_t write_fd(int fd,void *ptr,size_t nbytes,int sendfd);

int create_un_execl(const char *path,const char *child);

ssize_t read_fd(int fd,void *ptr,size_t nbytes,int *recvfd);
*/

struct hostent *Gethostbyaddr(const char *ip,int32_t family);

int32_t setNonblock(HANDLE sock);

#endif



