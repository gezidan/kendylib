#include "KendyNet.h"
#if defined(_LINUX)
#include "epoll.h"
#elif defined(_WIN)
#include "iocp.h"
#endif
#include "Engine.h"
#include "Socket.h"
#include "link_list.h"
#include "HandleMgr.h"
#include "SocketWrapper.h"
#include <assert.h>
#include "double_link.h"
void init_buff_allocator();
int32_t InitNetSystem()
{
	return 0;
}

int32_t EngineRun(ENGINE engine,int32_t timeout)
{
	engine_t e = GetEngineByHandle(engine);
	if(!e)
		return -1;
	return e->Loop(e,timeout);
}

ENGINE CreateEngine()
{
	ENGINE engine = NewEngine();
	if(engine != INVAILD_ENGINE)
	{
		engine_t e = GetEngineByHandle(engine);
		if(0 != e->Init(e))
		{
			CloseEngine(engine);
			engine = INVAILD_ENGINE;
		}
	}
	return engine;
}

void CloseEngine(ENGINE handle)
{
	ReleaseEngine(handle);
}

int32_t Bind2Engine(ENGINE e,SOCK s,OnRead _OnRead,OnWrite _OnWrite,OnClear_pending _OnClear_pending)
{
	engine_t engine = GetEngineByHandle(e);
	socket_t sock   = GetSocketByHandle(s);
	if(!engine || ! sock)
		return -1;
	sock->OnRead = _OnRead;
	sock->OnWrite = _OnWrite;
	sock->OnClear_pending_io = _OnClear_pending;
	sock->engine = engine;
	if(engine->Register(engine,sock) == 0)
		return 0;
	else
		sock->engine = NULL;
	return -1;
}

#if defined(_LINUX)

int32_t Recv(SOCK sock,st_io *io,uint32_t *err_code)
{
	assert(io);
	socket_t s = GetSocketByHandle(sock);
	if(!s)
	{
		*err_code = 0;
		return -1;
	}
	return raw_recv(s,io,err_code);
}

int32_t Post_Recv(SOCK sock,st_io *io)
{
	assert(io);
	socket_t s = GetSocketByHandle(sock);
	if(!s)
		return -1;
	LINK_LIST_PUSH_BACK(s->pending_recv,io);
	if(s->engine && s->readable && !s->isactived)
	{
		s->isactived = 1;
		double_link_push(s->engine->actived,(struct double_link_node*)s);
	}
	return 0;
}

int32_t Send(SOCK sock,st_io *io,uint32_t *err_code)
{
	assert(io);
	socket_t s = GetSocketByHandle(sock);
	if(!s)
	{
		*err_code = 0;
		return -1;
	}
	return raw_send(s,io,err_code);
}

int32_t Post_Send(SOCK sock,st_io *io)
{
	assert(io);
	socket_t s = GetSocketByHandle(sock);
	if(!s)
		return -1;
	LINK_LIST_PUSH_BACK(s->pending_send,io);
	if(s->engine && s->writeable && !s->isactived)
	{
		s->isactived = 1;
		double_link_push(s->engine->actived,(struct double_link_node*)s);
	}
	return 0;
}
#elif defined(_WIN)
#endif
/*
int32_t WSASend(HANDLE sock,st_io *io,int32_t flag,uint32_t *err_code)
{
	assert(io);
	socket_t s = GetSocketByHandle(sock);
	if(!s)
	{
		*err_code = 0;
		return -1;
	}
	if(flag == SEND_POST)
	{
		LINK_LIST_PUSH_BACK(s->pending_send,io);
		if(s->engine && s->writeable && !s->isactived)
		{
			s->isactived = 1;
			double_link_push(s->engine->actived,(struct double_link_node*)s);
			//LINK_LIST_PUSH_BACK(s->engine->actived,s);
		}
		*err_code = EAGAIN;
		return -1;
	}
	else
	{
		int32_t bytes_transfer = 0;
		return raw_send(s,io,&bytes_transfer,err_code);
	}
}

int32_t WSARecv(HANDLE sock,st_io *io,int32_t flag,uint32_t *err_code)
{
	assert(io);
	socket_t s = GetSocketByHandle(sock);
	if(!s)
	{
		*err_code = 0;
		return -1;
	}
	if(flag == RECV_POST)
	{
		LINK_LIST_PUSH_BACK(s->pending_recv,io);
		if(s->engine && s->readable && !s->isactived)
		{
			s->isactived = 1;
			double_link_push(s->engine->actived,(struct double_link_node*)s);
		}
		*err_code = EAGAIN;
		return -1;
	}
	else
	{
		int bytes_transfer = 0;
		return raw_recv(s,io,&bytes_transfer,err_code);
	}
}
*/