#ifndef _CONNECTION_H
#define _CONNECTION_H
#include "KendyNet.h"
#include "wpacket.h"
#include "rpacket.h"
#include "SocketWrapper.h"
#include <stdint.h>
#include "util/timing_wheel.h"
#include "util/allocator.h"
struct connection;
struct OVERLAPCONTEXT
{
	st_io m_super;
	int8_t   isUsed;
	struct connection *c;
};


typedef void (*process_packet)(struct connection*,rpacket_t);
typedef void (*on_disconnect)(struct connection*,int32_t reason);


#define MAX_WBAF 512
#define MAX_SEND_SIZE 4096

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
	on_disconnect _on_disconnect;
	uint8_t mt;
	uint8_t raw;
	uint16_t is_close;
	uint64_t usr_data;
	uint32_t last_recv;
	uint32_t recv_timeout;
	uint32_t send_timeout;
	WheelItem_t wheelitem;
	allocator_t rpacket_allocator;
	
};

struct connection *connection_create(HANDLE s,uint8_t is_raw,uint8_t mt,process_packet,on_disconnect);
void connection_active_close(struct connection*);//active close connection
int connection_destroy(struct connection**);

//仅仅把包放入发送队列
void connection_push_packet(struct connection*,wpacket_t,packet_send_finish);

//返回值:0,连接断开;否则正常
int32_t connection_send(struct connection*,wpacket_t,packet_send_finish);

int32_t connection_start_recv(struct connection*);

void SendFinish(int32_t bytetransfer,st_io *io);
void RecvFinish(int32_t bytetransfer,st_io *io);
#endif
