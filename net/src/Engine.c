#include "KendyNet.h"
#include "epoll.h"
#include "Engine.h"
#include "util/link_list.h"
#include <stdlib.h>
#include <assert.h>

engine_t create_engine()
{
	engine_t e = malloc(sizeof(*e));
	if(e)
	{
		e->status = 1;
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
