#ifndef _HANDLEMGR_H
#define _HANDLEMGR_H

#include "KendyNet.h"
#include <stdint.h>
struct socket_wrapper;
struct engine;

inline socket_t GetSocketByHandle(HANDLE handle)
{
	return (socket_t)handle;
}

inline engine_t GetEngineByHandle(HANDLE handle)
{
	return (engine_t)handle;
}


extern HANDLE   NewEngine();
extern void     ReleaseEngine(HANDLE);

extern HANDLE   NewSocketWrapper();
extern int32_t      ReleaseSocketWrapper(HANDLE);

#endif