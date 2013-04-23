#include "epoll.h"
#include "Socket.h"
#include "Engine.h"
#include "HandleMgr.h"

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