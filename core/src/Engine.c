#include "KendyNet.h"
#include "link_list.h"
#include <stdlib.h>
#include <assert.h>

#if defined(_LINUX)
#include "epoll.h"
#include "Engine.h"
engine_t create_engine()
{
	engine_t e = malloc(sizeof(*e));
	if(e)
	{
		e->Init = epoll_init;
		e->Loop = epoll_loop;
		e->Register = epoll_register;
		e->UnRegister = epoll_unregister;
		e->actived = (struct double_link*)calloc(1,sizeof(*e->actived));
		double_link_clear(e->actived);
	}
	return e;
}

void   free_engine(engine_t *e)
{
	assert(e);
	assert(*e);
	free((*e)->actived);
	free(*e);
	*e = 0;
}
#elif defined(_WIN)
#include "iocp.h"
#include "Engine.h"
engine_t create_engine()
{
	engine_t e = malloc(sizeof(*e));
	if(e)
	{
		e->Init = iocp_init;
		e->Loop = iocp_loop;
		e->Register = iocp_register;
		e->UnRegister = iocp_unregister;
	}
	return e;
}

void   free_engine(engine_t *e)
{
	free(*e);
	*e = 0;
}

#endif