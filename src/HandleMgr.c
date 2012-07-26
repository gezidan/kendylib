#include "epoll.h"
#include "Socket.h"
#include "Engine.h"
#include "HandleMgr.h"
#include "sync.h"

static mutex_t  engine_mtx;
static engine_t engine_pool[MAX_ENGINE];
static int32_t current_engine_count = 0;

static mutex_t  socket_mtx;
static socket_t socket_pool[MAX_SOCKET];
static int32_t current_socket_count = 0;

int32_t InitHandleMgr()
{	
	engine_mtx = mutex_create();
	socket_mtx = mutex_create();
	return 0;
	
}

inline socket_t GetSocketByHandle(HANDLE handle)
{
	if(handle >= 0 && handle < current_socket_count && socket_pool[handle]->status != 0)
		return socket_pool[handle];
	return 0;
}

inline engine_t GetEngineByHandle(HANDLE handle)
{
	if(handle >= 0 && handle < current_engine_count && engine_pool[handle]->status != 0)
		return engine_pool[handle];
	return 0;	
}

HANDLE	NewSocketWrapper()
{
	mutex_lock(socket_mtx);
	int32_t i = 0;
	int32_t cur_socket_count = current_socket_count;
	//首先查看已创建的socket中是否有可用的
	for( ; i < current_socket_count; ++i)
	{
		if(socket_pool[i]->status == 0)
		{
			socket_pool[i]->status = 1;
			break;
		}
	}
	if(i == current_socket_count && current_socket_count < MAX_SOCKET)
	{
		//还没到达上限,新产生一个socket
		socket_pool[current_socket_count] = create_socket();
		if(socket_pool[current_socket_count])
		{
			socket_pool[current_socket_count]->status = 1;
			socket_pool[current_socket_count]->handle = current_socket_count;
			cur_socket_count = ++current_socket_count;
		}
	}
	mutex_unlock(socket_mtx);
	if(i < cur_socket_count)
		return i;
	return -1;
}


inline static int32_t RemoveBinding(engine_t e, socket_t sock)
{
	return e ? e->UnRegister(e,sock) : -1;
}


int32_t  ReleaseSocketWrapper(HANDLE handle)
{
	int32_t ret = -1;
	mutex_lock(socket_mtx);
	if(handle >= 0 && handle < current_socket_count)
	{
		//mutex_lock(socket_pool[handle]->mtx);
		if(socket_pool[handle]->status != 0)
		{
			RemoveBinding(socket_pool[handle]->engine,socket_pool[handle]);
			close(socket_pool[handle]->fd);
			socket_pool[handle]->status = 0;
			ret = 0;			
		}
		//mutex_unlock(socket_pool[handle]->mtx);	
	}
	mutex_unlock(socket_mtx);
	return ret;
} 

HANDLE	NewEngine()
{
	mutex_lock(engine_mtx);
	int32_t i = 0;
	int32_t cur_engine_count = current_engine_count;
	for( ; i < current_engine_count; ++i)
	{
		if(engine_pool[i]->status == 0)
		{
			engine_pool[i]->status = 1;
			break;
		}
	}
	
	if(i == current_engine_count && current_engine_count < MAX_ENGINE)
	{
		//还没到达上限,新产生一个engine
		engine_pool[current_engine_count] = create_engine();
		if(engine_pool[current_engine_count])
		{
			engine_pool[current_engine_count]->status = 1;
			cur_engine_count = ++current_engine_count;
		}
	}
    	
	mutex_unlock(engine_mtx);
	
	if(i < cur_engine_count)
		return i;
	return -1;
}

void  ReleaseEngine(HANDLE handle)
{
	mutex_lock(engine_mtx);
	if(handle >= 0 && handle < current_engine_count && engine_pool[handle]->status != 0)
	{
		engine_pool[handle]->status = 0;
	}
	mutex_unlock(engine_mtx);
} 