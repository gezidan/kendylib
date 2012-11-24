#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include "SocketWrapper.h"
#include "KendyNet.h"
#include "epoll.h"
#include "Engine.h"
#include "Socket.h"

socket_t create_socket()
{
	socket_t s = malloc(sizeof(*s));
	if(s)
	{
		s->pending_send = LINK_LIST_CREATE();
		s->pending_recv = LINK_LIST_CREATE();
		s->status = 0;
		s->engine = 0;
		s->isactived = 0;
	}
	return s;
}

void free_socket(socket_t *s)
{
	assert(s);assert(*s);
	destroy_link_list(&(*s)->pending_send);
	destroy_link_list(&(*s)->pending_recv);
	free(*s);
	*s = 0;
}

void on_read_active(socket_t s)
{
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

int32_t  Process(socket_t s)
{
	_recv(s);
	_send(s);
	int32_t read_active = s->readable && !LINK_LIST_IS_EMPTY(s->pending_recv);
	int32_t write_active = s->writeable && !LINK_LIST_IS_EMPTY(s->pending_send);
	return (read_active || write_active) && s->isactived == 0;
}

int32_t raw_recv(socket_t s,st_io *io_req,int32_t *bytes_transfer,uint32_t *err_code)
{
	int32_t ret = 0;
	*err_code = 0;
	*bytes_transfer = TEMP_FAILURE_RETRY(readv(s->fd,io_req->iovec,io_req->iovec_count));
	if(*bytes_transfer < 0)
	{
		if(errno == EAGAIN)
		{
			s->readable = 0;
			//将请求重新放回到队列
			LINK_LIST_PUSH_FRONT(s->pending_recv,io_req);
		}
		*err_code = errno;
		ret = -1;
	}
	else if(*bytes_transfer == 0)
		ret = 0;
	else
		ret = *bytes_transfer;
	return ret;
}


void _recv(socket_t s)
{
	assert(s);
	int32_t ret = -1;
	int32_t bytes_transfer = 0;
	st_io* io_req = 0;
	if(s->readable)
	{
		if(io_req = LINK_LIST_POP(st_io*,s->pending_recv))
		{
			ret = raw_recv(s,io_req,&bytes_transfer,&io_req->err_code);
			if(ret >= 0 || io_req->err_code != EAGAIN)
				s->OnRead(bytes_transfer,io_req);		
		}
	}
}

int32_t raw_send(socket_t s,st_io *io_req,int32_t *bytes_transfer,uint32_t *err_code)
{
	int32_t ret = 0;
	*err_code = 0;
	*bytes_transfer = TEMP_FAILURE_RETRY(writev(s->fd,io_req->iovec,io_req->iovec_count));
	if(*bytes_transfer < 0)
	{
		if(errno == EAGAIN)
		{
			s->writeable = 0;
			//将请求重新放回到队列
			LINK_LIST_PUSH_FRONT(s->pending_send,io_req);
		}
		ret = -1;
		*err_code = errno;
	}
	else if(*bytes_transfer == 0)
		ret = 0;
	else
		ret = *bytes_transfer;
	return ret;
}

void _send(socket_t s)
{
	assert(s);
	int32_t ret = -1;
	int32_t bytes_transfer = 0;	
	st_io* io_req = 0;
	if(s->writeable)
	{
		if(io_req = LINK_LIST_POP(st_io*,s->pending_send))
		{	
			ret = raw_send(s,io_req,&bytes_transfer,&io_req->err_code);
			if(ret >= 0 || io_req->err_code != EAGAIN)
				s->OnWrite(bytes_transfer,io_req);
		}
	}	
}
