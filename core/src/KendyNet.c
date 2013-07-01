#include "KendyNet.h"
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
#ifdef _WIN
	int32_t nResult;
	WSADATA wsaData;
	nResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (NO_ERROR != nResult)
	{
		printf("\nError occurred while executing WSAStartup().");
		return -1; //error
	}
	return 0;
#else
	signal(SIGPIPE,SIG_IGN);
	return 0;
#endif
}


void   CleanNetSystem()
{
#ifdef _WIN
	WSACleanup();
#endif
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
	if(engine != INVALID_ENGINE)
	{
		engine_t e = GetEngineByHandle(engine);
		if(0 != e->Init(e))
		{
			CloseEngine(engine);
			engine = INVALID_ENGINE;
		}
	}
	return engine;
}

void CloseEngine(ENGINE handle)
{
	ReleaseEngine(handle);
}


#if defined(_LINUX)

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


int32_t Recv(SOCK sock,st_io *io,uint32_t *err_code)
{
	assert(io);
	socket_t s = GetSocketByHandle(sock);
	if(!s)
	{
		*err_code = 0;
		return -1;
	}
	int32_t ret = raw_recv(s,io,err_code);
	if(ret < 0 && *err_code == WSA_IO_PENDING)
		*err_code = EAGAIN;
	return ret;
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
	int32_t ret = raw_send(s,io,err_code);
	if(ret < 0 && *err_code == WSA_IO_PENDING)
		*err_code = EAGAIN;
	return ret;
}

void     iocp_post_request(engine_t e,void *ptr,st_io *io);

int32_t Post_Recv(SOCK sock,st_io *io)
{
	assert(io);
	socket_t s = GetSocketByHandle(sock);
	if(!s)
		return -1;
	io->m_Type = IO_RECVREQUEST;
	if(s->engine)
	{
		ZeroMemory(io, sizeof(OVERLAPPED));
		iocp_post_request(s->engine,(void*)s,io);
	}
	else
	{
		ZeroMemory(io, sizeof(list_node));
		LINK_LIST_PUSH_BACK(s->pending_recv,io);
	}
	return 0;
}

int32_t Post_Send(SOCK sock,st_io *io)
{
	assert(io);
	socket_t s = GetSocketByHandle(sock);
	if(!s)
		return -1;
	io->m_Type = IO_SENDREQUEST;
	if(s->engine)
	{
		ZeroMemory(io, sizeof(OVERLAPPED));
		iocp_post_request(s->engine,(void*)s,io);
	}
	else
	{
		ZeroMemory(io, sizeof(list_node));
		LINK_LIST_PUSH_BACK(s->pending_send,io);
	}
	return 0;
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
	{
        list_node *tmp;
        while((tmp = link_list_pop(sock->pending_send))!=NULL)
		{
			ZeroMemory(tmp, sizeof(OVERLAPPED));
			iocp_post_request(e,(void*)sock,(st_io*)tmp);
		}
		while((tmp = link_list_pop(sock->pending_recv))!=NULL)
		{    
			ZeroMemory(tmp, sizeof(OVERLAPPED));
			iocp_post_request(e,(void*)sock,(st_io*)tmp);
		}
		return 0;
	}
	else
		sock->engine = NULL;
	return -1;
}

#endif