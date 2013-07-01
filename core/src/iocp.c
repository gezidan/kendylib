#ifdef _WIN
#include <winsock2.h>
#include <WinBase.h>
#include <Winerror.h>
#include <stdint.h>
#include "Engine.h"
#include "Socket.h"
#include "SysTime.h"
#include <assert.h>
#include "KendyNet.h"

int32_t  iocp_init(engine_t e)
{
	e->complete_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	if(NULL == e->complete_port)
	{
		printf("\nError occurred while creating IOCP: %d.", WSAGetLastError());
		return -1;
	}
	return 0;
}

int32_t iocp_register(engine_t e, socket_t s)
{
	HANDLE hTemp;
	hTemp = CreateIoCompletionPort((HANDLE)s->fd, e->complete_port,(ULONG_PTR)s, 1);
	if (NULL == hTemp)
		return -1;
	s->engine = e;
	return 0;
}


int32_t iocp_unregister(engine_t e,socket_t s)
{
	s->engine = NULL;
	return 0;
}
typedef void (*CallBack)(int32_t,st_io*,uint32_t);
int32_t iocp_loop(engine_t n,int32_t timeout)
{
	assert(n);
	int32_t        bytesTransfer;
	socket_t       socket;
	st_io *overLapped = 0;
	uint32_t lastErrno = 0;
	BOOL bReturn;
	CallBack call_back;
	uint32_t ms;
	uint32_t tick = GetSystemMs();
	uint32_t _timeout = tick + timeout;
	do
	{
		ms = _timeout - tick;
		call_back = NULL;
		lastErrno = 0;
		bReturn = GetQueuedCompletionStatus(
			n->complete_port,(PDWORD)&bytesTransfer,
			(LPDWORD)&socket,
			(OVERLAPPED**)&overLapped,ms);

		if(FALSE == bReturn && !overLapped)// || socket == NULL || overLapped == NULL)
		{
			break;
		}
		if(0 == bytesTransfer)
		{
			//连接中断或错误
			lastErrno = WSAGetLastError();			
			if(bReturn == TRUE && lastErrno == 0)
			{
			}
			else
			{
				if(overLapped->m_Type & IO_RECV)
					call_back = socket->OnRead;	
				else
					call_back = socket->OnWrite;
				if(FALSE == bReturn)
					bytesTransfer = -1;
			}
		}
		else
		{	
			if(overLapped->m_Type & IO_REQUEST)
			{
				if(overLapped->m_Type  == IO_RECVREQUEST)
					bytesTransfer = raw_recv(socket,overLapped,&lastErrno);
				else if(overLapped->m_Type  == IO_SENDREQUEST)
					bytesTransfer = raw_send(socket,overLapped,&lastErrno);
				else
				{
					//出错
					continue;
				}
				if((bytesTransfer < 0 && lastErrno != WSA_IO_PENDING))
				{
					if(overLapped->m_Type  == IO_RECVREQUEST)
						call_back = socket->OnRead;
					else
						call_back = socket->OnWrite;
				}
			}
			else
			{
				if(overLapped->m_Type & IO_RECVFINISH)
					call_back = socket->OnRead;
				else if(overLapped->m_Type & IO_SENDFINISH)
					call_back = socket->OnWrite;
				else
				{
					//出错
					continue;
				}
			}
		}

		if(call_back)
			call_back(bytesTransfer,overLapped,lastErrno);
		tick = GetSystemMs();
	}while(tick < _timeout);
	return 0;
}

void     iocp_post_request(engine_t e,void *ptr,st_io *io)
{
	PostQueuedCompletionStatus(e->complete_port,1,(ULONG_PTR)ptr,(OVERLAPPED*)io);
}


#endif