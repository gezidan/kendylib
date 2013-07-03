#include "Connection.h"
#include "link_list.h"
#include <assert.h>
#include "SysTime.h"

#define BUFFER_SIZE 65536

//接收相关函数
static inline void update_next_recv_pos(struct connection *c,int32_t bytestransfer)
{
	uint32_t size;		
	do{
		size = c->next_recv_buf->capacity - c->next_recv_pos;
		size = size > (uint32_t)bytestransfer ? (uint32_t)bytestransfer:size;
		c->next_recv_buf->size += size;
		c->next_recv_pos += size;
		bytestransfer -= size;
		if(c->next_recv_pos >= c->next_recv_buf->capacity)
		{
			if(!c->next_recv_buf->next)
				c->next_recv_buf->next = buffer_create_and_acquire(c->mt,NULL,BUFFER_SIZE);
			c->next_recv_buf = buffer_acquire(c->next_recv_buf,c->next_recv_buf->next);
			c->next_recv_pos = 0;
		}
	}while(bytestransfer);
}

static inline void unpack(struct connection *c)
{
	uint32_t pk_len = 0;
	uint32_t pk_total_size;
	rpacket_t r;
	do{

		if(!c->raw)
		{		
			if(c->unpack_size <= sizeof(uint32_t))
				return;
			buffer_read(c->unpack_buf,c->unpack_pos,(int8_t*)&pk_len,sizeof(pk_len));
			pk_total_size = pk_len+sizeof(pk_len);
			if(pk_total_size > c->unpack_size)
				return;
			r = rpacket_create(c->mt,c->rpacket_allocator,c->unpack_buf,c->unpack_pos,pk_len,c->raw);
			//调整unpack_buf和unpack_pos
			do{
				uint32_t size = c->unpack_buf->size - c->unpack_pos;
				size = pk_total_size > size ? size:pk_total_size;
				c->unpack_pos  += size;
				pk_total_size  -= size;
				c->unpack_size -= size;
				if(c->unpack_pos >= c->unpack_buf->capacity)
				{
					/* unpack之前先执行了update_next_recv_pos,在update_next_recv_pos中
					*  如果buffer被填满，会扩展一块新的buffer加入链表中，所以这里的
					*  c->unpack_buf->next不应该会是NULL
					*/
					assert(c->unpack_buf->next);
					c->unpack_pos = 0;
					c->unpack_buf = buffer_acquire(c->unpack_buf,c->unpack_buf->next);
				}
			}while(pk_total_size);
		}
		else
		{
			pk_len = c->unpack_buf->size - c->unpack_pos;
			if(!pk_len)
				return; 
			r = rpacket_create(c->mt,NULL,c->unpack_buf,c->unpack_pos,pk_len,c->raw);
			c->unpack_pos  += pk_len;
			c->unpack_size -= pk_len;
			if(c->unpack_pos >= c->unpack_buf->capacity)
			{
				/* unpack之前先执行了update_next_recv_pos,在update_next_recv_pos中
				*  如果buffer被填满，会扩展一块新的buffer加入链表中，所以这里的
				*  c->unpack_buf->next不应该会是NULL
				*/
				assert(c->unpack_buf->next);
				c->unpack_pos = 0;
				c->unpack_buf = buffer_acquire(c->unpack_buf,c->unpack_buf->next);
			}
		}
		c->_process_packet(c,r);
		rpacket_destroy(&r);
	}while(1);
}

//发送相关函数
static inline st_io *prepare_send(struct connection *c)
{
	int32_t i = 0;
	wpacket_t w = (wpacket_t)link_list_head(c->send_list);
	buffer_t b;
	uint32_t pos;
	st_io *O = NULL;
	uint32_t buffer_size = 0;
	uint32_t size = 0;
	uint32_t send_size_remain = MAX_SEND_SIZE;
	while(w && i < MAX_WBAF && send_size_remain > 0)
	{
		pos = w->begin_pos;
		b = w->buf;
		buffer_size = w->data_size;
		while(i < MAX_WBAF && b && buffer_size && send_size_remain > 0)
		{
			c->wsendbuf[i].iov_base = b->buf + pos;
			size = b->size - pos;
			size = size > buffer_size ? buffer_size:size;
			size = size > send_size_remain ? send_size_remain:size;
			buffer_size -= size;
			send_size_remain -= size;
			c->wsendbuf[i].iov_len = size;
			++i;
			b = b->next;
			pos = 0;
		}
		if(send_size_remain > 0)
			w = (wpacket_t)w->next.next;
	}
	if(i)
	{
		c->send_overlap.m_super.iovec_count = i;
		c->send_overlap.m_super.iovec = c->wsendbuf;
		O = (st_io*)&c->send_overlap;
	}
	return O;

}
static inline void update_send_list(struct connection *c,int32_t bytestransfer)
{
	wpacket_t w;
	uint32_t size;
	while(bytestransfer)
	{
		w = LINK_LIST_POP(wpacket_t,c->send_list);
		assert(w);
		if((uint32_t)bytestransfer >= w->data_size)
		{
			//一个包发完
			bytestransfer -= w->data_size;
			if(w->_packet_send_finish)
				w->_packet_send_finish(c,w);
				
			wpacket_destroy(&w);
		}
		else
		{
			while(bytestransfer)
			{
				size = w->buf->size - w->begin_pos;
				size = size > (uint32_t)bytestransfer ? (uint32_t)bytestransfer:size;
				bytestransfer -= size;
				w->begin_pos += size;
				w->data_size -= size;
				if(w->begin_pos >= w->buf->size)
				{
					w->begin_pos = 0;
					w->buf = buffer_acquire(w->buf,w->buf->next);
				}
			}
			LINK_LIST_PUSH_FRONT(c->send_list,w);
		}
	}
}

int32_t connection_send(struct connection *c,wpacket_t w,packet_send_finish callback)
{
	if(c->is_closed != 0)
	{
		printf("is close\n");
		wpacket_destroy(&w);
		return -1;
	}
	st_io *O;
	if(w)
	{
		w->send_tick = GetSystemMs();
		w->_packet_send_finish = callback;
		LINK_LIST_PUSH_BACK(c->send_list,w);
	}
	if(!c->send_overlap.isUsed)
	{
		O = prepare_send(c);
		if(O)
		{
			c->send_overlap.isUsed = 1;
#ifdef _LINUX
			return Post_Send(c->socket,O);
#else
			uint32_t err_code = 0;
			int32_t bytestransfer = Send(c->socket,O,&err_code);
			if(bytestransfer < 0 && err_code != EAGAIN){
				//套接口断开，将请求post出去，在engine中统一处理断开事件
				return Post_Send(c->socket,O);
			}
			return bytestransfer;
#endif
		}
	}
	return 0;
}


struct connection *connection_create(SOCK s,uint8_t is_raw,uint8_t mt,process_packet _process_packet,on_disconnect _on_disconnect)
{
	struct connection *c = calloc(1,sizeof(*c));
	c->socket = s;
	c->send_list = LINK_LIST_CREATE();
	c->_process_packet = _process_packet;
	c->_on_disconnect = _on_disconnect;
	c->next_recv_buf = NULL;
	c->next_recv_pos = 0;
	c->unpack_buf = NULL;
	c->unpack_pos = 0;
	c->unpack_size = 0;
	c->recv_overlap.c = c;
	c->send_overlap.c = c;
	c->raw = is_raw;
	c->mt = mt;
	c->is_closed = 0;
	c->close_reason = 0;
	return c;
}

int connection_destroy(struct connection** c)
{
	if(!(*c)->recv_overlap.isUsed && !(*c)->send_overlap.isUsed)
	{ 
		wpacket_t w;
		while((w = LINK_LIST_POP(wpacket_t,(*c)->send_list))!=NULL)
			wpacket_destroy(&w);
		LINK_LIST_DESTROY(&(*c)->send_list);
		buffer_release(&(*c)->unpack_buf);
		buffer_release(&(*c)->next_recv_buf);
		if((*c)->wheelitem)
			DestroyWheelItem(&((*c)->wheelitem));
		free(*c);
		*c = NULL;
		printf("connection_destroy\n");
		return 0;
	}
	return -1;
}

int32_t connection_start_recv(struct connection *c)
{
	if(c->is_closed != 0 || c->unpack_buf != NULL)
		return -1;
	c->unpack_buf = buffer_create_and_acquire(c->mt,NULL,BUFFER_SIZE);
	c->next_recv_buf = buffer_acquire(NULL,c->unpack_buf);
	c->next_recv_pos = c->unpack_pos = c->unpack_size = 0;
	c->wrecvbuf[0].iov_len = BUFFER_SIZE;
	c->wrecvbuf[0].iov_base = c->next_recv_buf->buf;
	c->recv_overlap.m_super.iovec_count = 1;
	c->recv_overlap.isUsed = 1;
	c->recv_overlap.m_super.iovec = c->wrecvbuf;
	return Post_Recv(c->socket,&c->recv_overlap.m_super);
}

void connection_active_close(struct connection *c)
{
	if(c->is_closed == 0){
		c->is_closed = 1;
		CloseSocket(c->socket);
		printf("active close\n");
#ifdef _WIN		
		if(c->send_overlap.isUsed == 0 && c->recv_overlap.isUsed == 0)
		{
			if(c->_on_disconnect)
				c->_on_disconnect(c,-2);//-2,active colse
			else
				connection_destroy(&c);
		}
#else
		c->recv_overlap.isUsed = 0;
		if(c->send_overlap.isUsed == 0){
			if(c->_on_disconnect)
				c->_on_disconnect(c,-2);//-2,active colse
			else
				connection_destroy(&c);	
		}	
#endif			
	}
}

void RecvFinish(int32_t bytestransfer,st_io *io,uint32_t err_code)
{
	struct OVERLAPCONTEXT *OVERLAP = (struct OVERLAPCONTEXT *)io;
	struct connection *c = OVERLAP->c;
	uint32_t recv_size;
	uint32_t free_buffer_size;
	buffer_t buf;
	uint32_t pos;
	int32_t i = 0;
	do{
		if(bytestransfer == 0)
			return;
		else if(bytestransfer < 0 && err_code != EAGAIN){
			printf("recv close\n");
			c->recv_overlap.isUsed = 0;
			if(c->close_reason == 0) c->close_reason = err_code;
			if(c->is_closed == 0) c->is_closed = 1;
			if(!c->send_overlap.isUsed){
				if(c->_on_disconnect)
					c->_on_disconnect(c,c->close_reason);
				else
					connection_destroy(&c);				
			}
			return;
		}else if(bytestransfer > 0){
			int32_t total_size = 0;
			do{
				c->last_recv = GetSystemMs();
				update_next_recv_pos(c,bytestransfer);
				c->unpack_size += bytestransfer;
				total_size += bytestransfer;
				unpack(c);
				buf = c->next_recv_buf;
				pos = c->next_recv_pos;
				recv_size = BUFFER_SIZE/4;
				i = 0;
				do
				{
					free_buffer_size = buf->capacity - pos;
					free_buffer_size = recv_size > free_buffer_size ? free_buffer_size:recv_size;
					c->wrecvbuf[i].iov_len = free_buffer_size;
					c->wrecvbuf[i].iov_base = buf->buf + pos;
					recv_size -= free_buffer_size;
					pos += free_buffer_size;
					if(recv_size && pos >= buf->capacity)
					{
						pos = 0;
						if(!buf->next)
							buf->next = buffer_create_and_acquire(c->mt,NULL,BUFFER_SIZE);
						buf = buf->next;
					}
					++i;
				}while(recv_size);

				c->recv_overlap.isUsed = 1;
				c->recv_overlap.m_super.iovec_count = i;
				c->recv_overlap.m_super.iovec = c->wrecvbuf;
#if defined(_LINUX)
				if(total_size >= BUFFER_SIZE)
				{
					Post_Recv(c->socket,&c->recv_overlap.m_super);
					return;
				}
				else
					bytestransfer = Recv(c->socket,&c->recv_overlap.m_super,&err_code);
#elif defined(_WIN)
				bytestransfer = Recv(c->socket,&c->recv_overlap.m_super,&err_code);
				if(bytestransfer >= 0)
					return;
#endif
			}while(bytestransfer > 0);
		}
	}while(1);
}

void SendFinish(int32_t bytestransfer,st_io *io,uint32_t err_code)
{
	struct OVERLAPCONTEXT *OVERLAP = (struct OVERLAPCONTEXT *)io;
	struct connection *c = OVERLAP->c;
	do{
		if(bytestransfer == 0)
			return;
		else if(bytestransfer < 0 && err_code != EAGAIN){
			printf("send close\n");
			c->send_overlap.isUsed = 0;
			if(c->close_reason == 0) c->close_reason = err_code;
			if(c->is_closed == 0) c->is_closed = 1;
			if(!c->recv_overlap.isUsed){
				if(c->_on_disconnect)
					c->_on_disconnect(c,c->close_reason);
				else
					connection_destroy(&c);	
			}
			return;
		}else if(bytestransfer > 0)
		{
			do{
				update_send_list(c,bytestransfer);
				io = prepare_send(c);
				if(!io)
				{				
					//没有数据需要发送了
					c->send_overlap.isUsed = 0;
					if(c->is_closed != 0 && c->recv_overlap.isUsed == 0){
						//连接已经关闭
						if(c->_on_disconnect)
							c->_on_disconnect(c,c->close_reason);
						else
							connection_destroy(&c);	
					}					
					return;
				}
				bytestransfer = Send(c->socket,io,&err_code);
#ifdef _WIN
				if(bytestransfer >= 0)
					return;
#endif
			}while(bytestransfer > 0);
		}
	}while(1);
}
