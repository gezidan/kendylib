#include "HandleMgr.h"

ENGINE	NewSocketWrapper()
{
	return (ENGINE)create_socket();
}


inline static int RemoveBinding(engine_t e, socket_t sock)
{
	return e ? e->UnRegister(e,sock) : -1;
}

void  ReleaseSocketWrapper(SOCK handle)
{
	socket_t s = (socket_t)handle;
#if defined(_LINUX)
	double_link_remove((struct double_link_node*)s);
	if(s->engine)
		RemoveBinding(s->engine,s);
	close(s->fd);
#elif defined(_WIN)
	closesocket(s->fd);
#endif
	if(s->OnClear_pending_io)
	{
		list_node *tmp;
		while((tmp = link_list_pop(s->pending_send))!=NULL)
			s->OnClear_pending_io((st_io*)tmp);
		while((tmp = link_list_pop(s->pending_recv))!=NULL)
			s->OnClear_pending_io((st_io*)tmp);
	}
	free_socket(&s);
} 

ENGINE	NewEngine()
{
	return (ENGINE)create_engine();
}

void  ReleaseEngine(ENGINE handle)
{
	engine_t e = (engine_t)handle;
	free_engine(&e);
}
