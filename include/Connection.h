#ifndef _CONNECTION_H
#define _CONNECTION_H
#include "KendyNet.h"
#include "wpacket.h"
#include "rpacket.h"
#include "SocketWrapper.h"
#include <stdint.h>
struct connection;
struct OVERLAPCONTEXT
{
	st_io m_super;
	int8_t   isUsed;
	struct connection *c;
};


typedef void (*process_packet)(struct connection*,rpacket_t);
typedef void (*on_connection_destroy)(struct connection*);

#define MAX_WBAF 1024

struct connection
{
	HANDLE socket;
	struct iovec wsendbuf[MAX_WBAF];
	struct iovec wrecvbuf[2];
	
	struct OVERLAPCONTEXT send_overlap;
	struct OVERLAPCONTEXT recv_overlap;

	uint32_t unpack_size; //还未解包的数据大小

	uint32_t unpack_pos;
	uint32_t next_recv_pos;

	buffer_t next_recv_buf;
	buffer_t unpack_buf; 
	
	struct link_list *send_list;//待发送的包
	process_packet _process_packet;
	on_connection_destroy _on_destroy;
	uint8_t raw;
};

struct connection *connection_create(HANDLE s,uint8_t is_raw,process_packet,on_connection_destroy);
void connection_destroy(struct connection**);

//仅仅把包放入发送队列
void connection_push_packet(struct connection*,wpacket_t);

//返回值:0,连接断开;否则正常
int32_t connection_send(struct connection*,wpacket_t,int32_t);

int32_t connection_recv(struct connection*);

void SendFinish(int32_t bytetransfer,st_io *io);
void RecvFinish(int32_t bytetransfer,st_io *io);

#endif
