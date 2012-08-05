#include "log.h"
#include "link_list.h"
#include "sync.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "SocketWrapper.h"
#include "atomic.h"
#include "thread.h"
#include <assert.h>
#include "SysTime.h"
#include <stdio.h>
#include <errno.h>
#include "allocator.h"
#include "wpacket.h"
#define max_write_buf 1024
static const uint32_t max_log_filse_size = 1024*1024*100;//超过100MB,更换文件
extern uint32_t log_count;

struct log
{
	struct list_node lnode;
	int32_t file_descriptor;
	mutex_t mtx;
	struct link_list *log_queue;
	struct link_list *pending_log;
	uint64_t file_size;
	struct iovec wbuf[max_write_buf];
};

typedef struct log_system
{
	mutex_t mtx;
	struct  link_list *log_files;
	thread_t worker_thread;
	atomic_8_t is_close;
	int32_t    last_tick;
	int32_t    bytes;
	allocator_t _wpacket_allocator;
}*log_system_t;

static log_system_t g_log_system;

static void  destroy_log(log_t *l);

static void *worker_routine(void*);

int32_t	init_log_system()
{
	if(!g_log_system)
	{
		g_log_system = calloc(1,sizeof(*g_log_system));
		g_log_system->mtx = mutex_create();
		g_log_system->is_close = 0;
		g_log_system->log_files = create_link_list();
		g_log_system->worker_thread = CREATE_THREAD_RUN(1,worker_routine,0);
		g_log_system->last_tick = GetSystemMs();
		g_log_system->bytes = 0;
		g_log_system->_wpacket_allocator = NULL;
		return 0;
	}
	return -1;
}

void close_log_system()
{
	COMPARE_AND_SWAP(&(g_log_system->is_close),0,1);
	//停止写日志线程,并等待结束
	//g_log_system->is_close = 1;
	thread_join(g_log_system->worker_thread);
	mutex_lock(g_log_system->mtx);
	while(!link_list_is_empty(g_log_system->log_files))
	{
		log_t l = LINK_LIST_POP(log_t,g_log_system->log_files);
		destroy_log(&l);
	}
	mutex_unlock(g_log_system->mtx);	
	mutex_destroy(&g_log_system->mtx);
	destroy_link_list(&g_log_system->log_files);
	destroy_thread(&g_log_system->worker_thread);
	//DESTROY(&(g_log_system->_wpacket_allocator));
	free(g_log_system);
	g_log_system = 0;
}

static inline int32_t prepare_write(log_t l)
{
	int32_t i = 0;
	wpacket_t w = (wpacket_t)link_list_head(l->pending_log);
	buffer_t b;
	uint32_t pos;
	uint32_t buffer_size = 0;
	uint32_t size = 0;
	while(w && i < max_write_buf)
	{
		pos = w->begin_pos;
		b = w->buf;
		buffer_size = w->data_size;
		while(i < max_write_buf && b && buffer_size)
		{
			l->wbuf[i].iov_base = b->buf + pos;
			size = b->size - pos;
			size = size > buffer_size ? buffer_size:size;
			buffer_size -= size;
			l->wbuf[i].iov_len = size;
			++i;
			b = b->next;
			pos = 0;
		}
		w = (wpacket_t)w->next.next;
	}
	return i;
}

static inline void on_write_finish(log_t l,int32_t bytestransfer)
{
	wpacket_t w;
	uint32_t size;
	while(bytestransfer)
	{
		w = LINK_LIST_POP(wpacket_t,l->pending_log);
		assert(w);
		if((uint32_t)bytestransfer >= w->data_size)
		{
			//一个wpacket写完了
			bytestransfer -= w->data_size;
			++log_count;
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
			LINK_LIST_PUSH_FRONT(l->pending_log,w);
		}
	}
}

static uint32_t last_tick = 0;

static void write_to_file(log_t l,int32_t is_close)
{
	mutex_lock(l->mtx);
	if(!list_is_empty(l->log_queue))
		link_list_swap(l->pending_log,l->log_queue);
	mutex_unlock(l->mtx);
	if(is_close)
	{
		//日志系统关闭,写入关闭消息
		char buf[4096];
		time_t t = time(NULL);
		struct tm re;
		struct tm *_tm = localtime_r(&t,&re);
		snprintf(buf,4096,"[%d-%d-%d %d:%d:%d]close log sucessful\n",_tm->tm_year+1900,_tm->tm_mon+1,_tm->tm_mday,_tm->tm_hour,_tm->tm_min,_tm->tm_sec);
		int32_t str_len = strlen(buf);
		wpacket_t w = wpacket_create(0,NULL,str_len,1);
		wpacket_write_binary(w,buf,str_len);	
		LINK_LIST_PUSH_BACK(l->pending_log,w);
	}
	while(!list_is_empty(l->pending_log))
	{
		int32_t wbuf_count = prepare_write(l);
		int32_t bytetransfer = TEMP_FAILURE_RETRY(writev(l->file_descriptor,l->wbuf,wbuf_count));
		if(bytetransfer > 0)
		{
			l->file_size += bytetransfer;
			on_write_finish(l,bytetransfer);
			g_log_system->bytes += bytetransfer;
		}
		if(bytetransfer <= 0)
		{
			printf("errno: %d wbuf_count: %d\n",errno,wbuf_count);
		}
		if(last_tick +1000 <= GetSystemMs())
		{
			printf("log/ms:%u\n",log_count);
			last_tick = GetSystemMs();
			log_count = 0;
		}
	}
	
	if(!is_close)
	{
		/*
		* if(l->file_size > max_log_filse_size)
		* 文件超过一定大小,将原文件重命名,开一个新的文件
		*/
	}
}


log_t create_log(const char *path)
{
   log_t l = calloc(1,sizeof(*l));
   
   l->file_descriptor = open(path,O_RDWR|O_CREAT,S_IWUSR|S_IRUSR);
   if(l->file_descriptor < 0)
   {
	   free(l);
	   l = 0;
   }
   else
   {
		l->file_size = 0;
		l->mtx = mutex_create();
		l->log_queue = create_link_list();
		l->pending_log = create_link_list();
		//add to log system
		mutex_lock(g_log_system->mtx);
		LINK_LIST_PUSH_BACK(g_log_system->log_files,l);
		mutex_unlock(g_log_system->mtx);
		log_write(l,"open log file",1);
   }
   return l;			
}

static void  destroy_log(log_t *l)
{
	mutex_lock((*l)->mtx);
	while(!link_list_is_empty((*l)->log_queue))
	{
		wpacket_t w = LINK_LIST_POP(wpacket_t,(*l)->log_queue);
		wpacket_destroy(&w);
	}
	mutex_unlock((*l)->mtx);
	close((*l)->file_descriptor);
	mutex_destroy(&(*l)->mtx);
	destroy_link_list(&(*l)->log_queue);
	destroy_link_list(&(*l)->pending_log);
	free(*l);
	*l = 0;
}

int32_t log_write(log_t l,const char *content,int32_t level)
{
	if(g_log_system->is_close)
		return -1;
		
	char buf[4096];	
	time_t t = time(NULL);
	struct tm re;
	struct tm *_tm = localtime_r(&t,&re);
	snprintf(buf,4096,"[%d-%d-%d %d:%d:%d]%s\n",_tm->tm_year+1900,_tm->tm_mon+1,_tm->tm_mday,_tm->tm_hour,_tm->tm_min,_tm->tm_sec,content);
	//snprintf(buf,4096,"%s\n",content);
	int32_t str_len = strlen(buf);
	wpacket_t w = wpacket_create(0,NULL,str_len,1);
	wpacket_write_binary(w,buf,str_len);
	mutex_lock(l->mtx);
	LINK_LIST_PUSH_BACK(l->log_queue,w);
	mutex_unlock(l->mtx);
	return 0;
}

static inline void write_all_log_file(int32_t is_close)
{
	log_t l = (log_t)link_list_head(g_log_system->log_files);
	while(l)
	{
		write_to_file(l,is_close);
		l = (log_t)(((struct list_node*)l)->next);
	}
}

static void *worker_routine(void *arg)
{
	while(!g_log_system->is_close)
	{
		int32_t tick = GetSystemMs(); 
		write_all_log_file(0);
		tick = GetSystemMs() - tick;
		if(tick < 50)
			usleep(50-tick);
		if(last_tick +1000 <= GetSystemMs())
		{
			printf("log/ms:%u\n",log_count);
			last_tick = GetSystemMs();
			log_count = 0;
		}				
			
	}
	write_all_log_file(1);
	return 0;
}

