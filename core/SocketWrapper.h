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
# Brief:    һЩ�׽ӿ�API�İ�װ����.     
*/

#ifndef _SOCK_WRAPPER_H
#define _SOCK_WRAPPER_H

#include "common.h"

enum sock_family
{
	INET = AF_INET,
	INET6 = AF_INET6,
#ifdef _LINUX
	LOCAL = AF_LOCAL,
	ROUTE = AF_ROUTE,
	KEY = AF_KEY,
#endif
};

enum sock_type
{
	STREAM = SOCK_STREAM,//��Э��
	DGRAM  = SOCK_STREAM,//���ݱ�Э��
	SEQPACKET = SOCK_SEQPACKET,
	RAW = SOCK_RAW,//ԭʼ�׽ӿ�
};

enum sock_protocol
{
	TCP = IPPROTO_TCP,
	UDP = IPPROTO_UDP,
#ifdef _LINUX
	SCTP = IPPROTO_SCTP,
#endif
};


SOCK  OpenSocket(int32_t family,int32_t type,int32_t protocol);

//close connection
int32_t CloseSocket(SOCK);

//release handle
void ReleaseSocket(SOCK);

int32_t Connect(SOCK sock,const struct sockaddr *servaddr,socklen_t addrlen);

/*
 * brief: �����׽���,��������ĶԶ˽�������.
 * para:  
 *        ip:�Զ˵�IP��ַ
 *        port:�Զ˶˿�
 *        servaddr:
 *        retry: ���connectʧ���Ƿ����³���,ֱ�����ӳɹ��ŷ��� 
 * return: >0 �׽���,-1,ʧ��.
 */
SOCK Tcp_Connect(const char *ip,uint16_t port,struct sockaddr_in *servaddr,int32_t retry);

int32_t Bind(SOCK sock,const struct sockaddr *myaddr,socklen_t addrlen);

int32_t Listen(SOCK sock,int32_t backlog);

/*
 * brief: �����׽���,����,Ȼ���ڴ��׽����ϼ���.
 *
 */
SOCK Tcp_Listen(const char *ip,uint16_t port,struct sockaddr_in *servaddr,int32_t backlog);


SOCK Accept(SOCK,struct sockaddr *sa,socklen_t *salen);

/*
 * brief: ��ȡԶ�����ӵ�IP,�˿ں�.
 */
int32_t getRemoteAddrPort(SOCK sock,char *buf,uint16_t *port);
int32_t getLocalAddrPort(SOCK sock,struct sockaddr_in *remoAddr,socklen_t *len,char *buf,uint16_t *port);

#ifdef _LINUX
ssize_t write_fd(int fd,void *ptr,size_t nbytes,int sendfd);

int create_un_execl(const char *path,const char *child);

ssize_t read_fd(int fd,void *ptr,size_t nbytes,int *recvfd);
#endif

//struct hostent *Gethostbyaddr(const char *ip,int32_t family);

int32_t setNonblock(SOCK sock);

#endif



