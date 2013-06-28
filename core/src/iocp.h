#include "Engine.h"
#include "Socket.h"
#include <stdint.h>
int32_t  iocp_init(engine_t);
int32_t  iocp_loop(engine_t,int32_t timeout);
int32_t  iocp_register(engine_t,socket_t);
int32_t  iocp_unregister(engine_t,socket_t);
void     iocp_post_request(engine_t,void*,st_io*);