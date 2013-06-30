#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include "KendyNet.h"
#include "SocketWrapper.h"
#include "Engine.h"
#include "Socket.h"
#include <stdio.h>

#if defined(_LINUX)

socket_t create_socket()
{
	socket_t s = malloc(sizeof(*s));
	if(s)
	{
		s->pending_send = LINK_LIST_CREATE();
		s->pending_recv = LINK_LIST_CREATE();
		s->engine = NULL;
		s->isactived = 0;
	}
	return s;
}

void free_socket(socket_t *s)
{
	assert(s);assert(*s);
	if((*s)->OnClear_pending_io)
	{
        list_node *tmp;
        while((tmp = link_list_pop((*s)->pending_send))!=NULL)
            (*s)->OnClear_pending_io((st_io*)tmp);
        while((tmp = link_list_pop((*s)->pending_recv))!=NULL)
            (*s)->OnClear_pending_io((st_io*)tmp);
	}
	destroy_link_list(&(*s)->pending_send);
	destroy_link_list(&(*s)->pending_recv);
	free(*s);
	*s = 0;
}

void on_read_active(socket_t s)
{
	printf("on_read_active\n");
	s->readable = 1;
	if(!s->isactived && !LINK_LIST_IS_EMPTY(s->pending_recv))
	{
		s->isactived = 1;
		double_link_push(s->engine->actived,(struct double_link_node*)s);
	}
}

void on_write_active(socket_t s)
{
	s->writeable = 1;
	if(!s->isactived && !LINK_LIST_IS_EMPTY(s->pending_send))
	{
		s->isactived = 1;
		double_link_push(s->engine->actived,(struct double_link_node*)s);
	}
}


int32_t raw_recv(socket_t s,st_io *io_req,uint32_t *err_code)
{
	*err_code = 0;
	int32_t ret  = TEMP_FAILURE_RETRY(readv(s->fd,io_req->iovec,io_req->iovec_count));
	if(ret < 0)
	{
		*err_code = errno;
		if(*err_code == EAGAIN)
		{
			s->readable = 0;
			//将请求重新放回到队列
			LINK_LIST_PUSH_FRONT(s->pending_recv,io_req);
		}
	}
	return ret;
}


static inline void _recv(socket_t s)
{
	assert(s);
	st_io* io_req = 0;
	uint32_t err_code = 0;
	if(s->readable)
	{
		if((io_req = LINK_LIST_POP(st_io*,s->pending_recv))!=NULL)
		{
			int32_t bytes_transfer = raw_recv(s,io_req,&err_code);
			if(err_code != EAGAIN)
				s->OnRead(bytes_transfer,io_req,err_code);
		}
	}
}

int32_t raw_send(socket_t s,st_io *io_req,uint32_t *err_code)
{
	*err_code = 0;
	int32_t ret  = TEMP_FAILURE_RETRY(writev(s->fd,io_req->iovec,io_req->iovec_count));
	if(ret < 0)
	{
		*err_code = errno;
		if(*err_code == EAGAIN)
		{
			s->writeable = 0;
			//将请求重新放回到队列
			LINK_LIST_PUSH_FRONT(s->pending_send,io_req);
		}
	}
	return ret;
}

static inline void _send(socket_t s)
{
	assert(s);
	st_io* io_req = 0;
	uint32_t err_code = 0;
	if(s->writeable)
	{
		if((io_req = LINK_LIST_POP(st_io*,s->pending_send))!=NULL)
		{
			int32_t bytes_transfer = raw_send(s,io_req,&err_code);
			if(err_code != EAGAIN)
				s->OnWrite(bytes_transfer,io_req,err_code);
		}
	}
}

int32_t  Process(socket_t s)
{
	_recv(s);
	_send(s);
	int32_t read_active = s->readable && !LINK_LIST_IS_EMPTY(s->pending_recv);
	int32_t write_active = s->writeable && !LINK_LIST_IS_EMPTY(s->pending_send);
	return (read_active || write_active) && s->isactived == 0;
}

#elif defined(_WIN)

socket_t create_socket()
{
	socket_t s = malloc(sizeof(*s));
	if(s){
		s->engine = NULL;
		s->pending_send = LINK_LIST_CREATE();
		s->pending_recv = LINK_LIST_CREATE();
	}
	return s;
}

void free_socket(socket_t *s)
{
	assert(s);assert(*s);
	if((*s)->OnClear_pending_io)
	{
        list_node *tmp;
        while((tmp = link_list_pop((*s)->pending_send))!=NULL)
            (*s)->OnClear_pending_io((st_io*)tmp);
        while((tmp = link_list_pop((*s)->pending_recv))!=NULL)
            (*s)->OnClear_pending_io((st_io*)tmp);
	}
	destroy_link_list(&(*s)->pending_send);
	destroy_link_list(&(*s)->pending_recv);	
	free(*s);
	*s = 0;
}


/* return:
*  >  0 :bytestransfer
*  == 0 :WSA_IO_PENDING
*  <  0 :error or socket close
*/
int32_t raw_send(socket_t s,st_io *io_req,uint32_t *err_code)
{
	uint32_t dwFlags = 0;
	uint32_t dwBytes = 0;
	io_req->m_Type = IO_SENDFINISH;
	ZeroMemory(io_req, sizeof(OVERLAPPED));
	if(SOCKET_ERROR == WSASend(s->fd, io_req->iovec,io_req->iovec_count, 
		(LPDWORD)&dwBytes, dwFlags, (OVERLAPPED*)io_req, NULL))
	{
		if(err_code)
			*err_code = WSAGetLastError();
		return -1;
	}	
	else 
		return dwBytes;
}

int32_t raw_recv(socket_t s,st_io *io_req,uint32_t *err_code)
{
	uint32_t dwFlags = 0;
	uint32_t dwBytes = 0;
	int32_t  ret = 0;
	io_req->m_Type = IO_RECVFINISH;
	ZeroMemory(io_req, sizeof(OVERLAPPED));
	if(SOCKET_ERROR == WSARecv(s->fd, io_req->iovec,io_req->iovec_count, 
		(LPDWORD)&dwBytes, (LPDWORD)&dwFlags, (OVERLAPPED*)io_req, NULL))
	{
		if(err_code)
			*err_code = WSAGetLastError();
		return -1;
	}	
	else 
		return dwBytes;
}


#endif
