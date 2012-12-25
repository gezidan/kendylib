#ifndef _HANDLEMGR_H
#define _HANDLEMGR_H

#include "KendyNet.h"
#include <stdint.h>
struct socket_wrapper;
struct engine;

extern int32_t     InitHandleMgr();

inline extern struct socket_wrapper* GetSocketByHandle(HANDLE);
inline extern struct engine* GetEngineByHandle(HANDLE);

extern HANDLE   NewEngine();
extern void     ReleaseEngine(HANDLE);

extern HANDLE   NewSocketWrapper();
extern int32_t      ReleaseSocketWrapper(HANDLE);

#endif