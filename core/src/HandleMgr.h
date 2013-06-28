#ifndef _HANDLEMGR_H
#define _HANDLEMGR_H

#include "KendyNet.h"
#include "Socket.h"
#include "Engine.h"
#include <stdint.h>
struct socket_wrapper;
struct engine;

static inline socket_t GetSocketByHandle(SOCK handle)
{
	return (socket_t)handle;
}

static inline engine_t GetEngineByHandle(ENGINE handle)
{
	return (engine_t)handle;
}


extern ENGINE   NewEngine();
extern void     ReleaseEngine(ENGINE);

extern SOCK   NewSocketWrapper();
extern void     ReleaseSocketWrapper(SOCK);

#endif
