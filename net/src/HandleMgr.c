#include "epoll.h"
#include "Socket.h"
#include "Engine.h"
#include "HandleMgr.h"
#include "util/sync.h"

static mutex_t  engine_mtx;
static engine_t engine_pool[MAX_ENGINE];
static int current_engine_count = 0;

static mutex_t  socket_mtx;
static socket_t socket_pool[MAX_SOCKET];
static int current_socket_count = 0;

int InitHandleMgr()
{	
	engine_mtx = mutex_create();
	socket_mtx = mutex_create();
	return 0;
	
}

inline socket_t GetSocketByHandle(HANDLE handle)
{
	return (socket_t)handle;
}

inline engine_t GetEngineByHandle(HANDLE handle)
{
	return (engine_t)handle;
}

HANDLE	NewSocketWrapper()
{
	return (HANDLE)create_socket();
}


inline static int RemoveBinding(engine_t e, socket_t sock)
{
	return e ? e->UnRegister(e,sock) : -1;
}

int  ReleaseSocketWrapper(HANDLE handle)
{
	socket_t s = (socket_t)handle;
	double_link_remove((struct double_link_node*)s);
	if(s->engine)
		RemoveBinding(s->engine,s);
	close(s->fd);
	if(s->OnClear_pending_io)
	{
		list_node *tmp;
		while(tmp = link_list_pop(s->pending_send))
			s->OnClear_pending_io((st_io*)tmp);
		while(tmp = link_list_pop(s->pending_recv))
			s->OnClear_pending_io((st_io*)tmp);
	}	
	free_socket(&s);
} 

HANDLE	NewEngine()
{
	return (HANDLE)create_engine();
}

void  ReleaseEngine(HANDLE handle)
{
	engine_t e = (engine_t)handle;
	free_engine(&e);
}