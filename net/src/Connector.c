#include <stdio.h>
#include "util/link_list.h"
#include <sys/select.h>
#include <sys/time.h>
#include "SocketWrapper.h"
#include "Socket.h"
#include "HandleMgr.h" 
#include "util/SysTime.h"
#include "Connector.h"
#include "util/sync.h"

typedef struct pending_connect
{
	list_node  lnode;
	const char *ip;
	uint32_t port;
	HANDLE   sock;
	int real_fd;
	on_connect call_back;
	uint32_t timeout;
	void     *ud;
}pending_connect;

struct connector
{
	fd_set Set;
	struct link_list *_pending_connect;
	uint32_t fd_seisize;
	mutex_t  lock;
	struct link_list *extern_pending_connect; 
};

connector_t connector_create()
{
	connector_t c = malloc(sizeof(*c));
	c->fd_seisize = 0;
	FD_ZERO(&c->Set);
	c->_pending_connect = create_link_list();
	c->extern_pending_connect = create_link_list();
	c->lock = mutex_create();
	return c;
}

void connector_destroy(connector_t *c)
{
	struct pending_connect *pc;
	while(pc = LINK_LIST_POP(struct pending_connect*,(*c)->_pending_connect))
		free(pc);
	while(pc = LINK_LIST_POP(struct pending_connect*,(*c)->extern_pending_connect))
		free(pc);
	mutex_destroy(&((*c)->lock));	
	free(*c);
	*c = 0;
}

int32_t connector_connect(connector_t c,const char *ip,uint32_t port,on_connect call_back,void *ud,uint32_t ms)
{
	struct sockaddr_in remote;
	HANDLE sock;
	struct pending_connect *pc;	
	sock = OpenSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock < 0)
		return -1;
	
	remote.sin_family = AF_INET;
	remote.sin_port = htons(port);
	if(inet_pton(INET,ip,&remote.sin_addr) < 0)
	{

		printf("%s\n",strerror(errno));
		return -1;
	}
	if(ms>0)
	{
		if(setNonblock(sock)!=0)
			return -1;
	}
	if(Connect(sock, (struct sockaddr *)&remote, sizeof(remote)) == 0)
	{
		//连接成功,无需要后续处理了,直接调用回调函数
		call_back(sock,ip,port,ud);
		return 0;
	}
	
	socket_t s = GetSocketByHandle(sock);

	pc = malloc(sizeof(*pc));
	pc->lnode.next = NULL;
	pc->sock = sock;
	pc->ip = ip;
	pc->port = port;
	pc->call_back = call_back;
	pc->timeout = GetSystemMs() + ms;
	pc->ud = ud;
	pc->real_fd = s->fd;
	mutex_lock(c->lock);
	LINK_LIST_PUSH_BACK(c->extern_pending_connect,pc);
	mutex_unlock(c->lock);
	return 0;
}

void connector_run(connector_t c,uint32_t ms)
{
	int32_t i = 0;
	uint32_t tick,_timeout,_ms;
	int32_t size;
	int32_t total;
	struct pending_connect *pc;
	struct timeval timeout;
	tick = GetSystemMs();
	_timeout = tick + ms;
	
	struct link_list *_l = LINK_LIST_CREATE();
	mutex_lock(c->lock);
	link_list_swap(_l,c->extern_pending_connect);
	mutex_unlock(c->lock);
	while(pc = LINK_LIST_POP(struct pending_connect*,_l))
	{
		if(c->fd_seisize >= FD_SETSIZE)
		{
			pc->call_back(-1,pc->ip,pc->port,pc->ud);
			free(pc);
		}
		else
		{
			FD_SET(pc->real_fd,&c->Set);
			LINK_LIST_PUSH_BACK(c->_pending_connect,pc);
			++c->fd_seisize;
		}
	}
	LINK_LIST_DESTROY(&_l);
	
	do{
		_ms = _timeout - tick;
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000*_ms;
		size = list_size(c->_pending_connect);
		if(size == 0)
			return;
		if((total = select(1024,0,&c->Set,0, &timeout)) >0 )
		{
			for(; i < size; ++i)
			{
				pc = LINK_LIST_POP(struct pending_connect*,c->_pending_connect);
				if(pc)
				{
					if(FD_ISSET(pc->real_fd, &c->Set))
					{
						pc->call_back(pc->sock,pc->ip,pc->port,pc->ud);
						free(pc);
						--c->fd_seisize;
					}
					else
						LINK_LIST_PUSH_BACK(c->_pending_connect,pc);
				}
			}
		}
		FD_ZERO(&c->Set);
		tick = GetSystemMs();
		size = list_size(c->_pending_connect);
		i = 0;
		for(; i < (int32_t)size; ++i)
		{
			pc = LINK_LIST_POP(struct pending_connect*,c->_pending_connect);
			if(tick >= pc->timeout)
			{
				pc->call_back(-1,pc->ip,pc->port,pc->ud);
				free(pc);
				--c->fd_seisize;
			}
			else
			{
				LINK_LIST_PUSH_BACK(c->_pending_connect,pc);
				FD_SET(pc->real_fd,&c->Set);
			}
		}
		tick = GetSystemMs();
	}while(tick < _timeout);
}
