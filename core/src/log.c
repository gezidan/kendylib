#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include "log.h"
#include "link_list.h"
#include "sync.h"
#include "atomic_st.h"
#include "thread.h"
#include "common.h"
#include "SysTime.h"
#ifdef _WIN
#include <sys/stat.h>
#endif

#define max_size 65536
static const uint32_t max_log_filse_size = 1024*1024*100;

static const char *level_str[LEV_SIZE] =
{
	"[error]",
	"[critical]",
	"[warning]", 
	"[message]",
	"[info]",
	"[debug]",
};

//1+10+22+1,最少也要容纳等级和时间的空间
#define MIN_STR_LEN 34 

//日期格式"YYYY-MM-DD HH:MM:SS"
typedef struct log_str
{
	struct list_node lnode;
	uint32_t len;
	uint32_t start_pos;
	char str[MIN_STR_LEN];
}*log_str_t;

struct log
{
	struct list_node lnode;
	int32_t file_descriptor;
	mutex_t mtx;
	struct link_list *log_queue;
	struct link_list *pending_write;//等待写入到磁盘中的日志
	uint64_t file_size;
	char write_buffer[max_size];
};


struct sys_time_str
{
	struct atomic_st base;
	char time_str[22]; 
};


GET_ATOMIC_ST(GetTimeStr,struct sys_time_str);	
SET_ATOMIC_ST(SetTimeStr,struct sys_time_str);

void update_time_str(struct atomic_type *time_str)
{
	if(time_str){
		struct sys_time_str tmp;
		GetCurrentTimeStr(tmp.time_str);		
		SetTimeStr(time_str,&tmp);
	}else
		printf("un init\n");
}

typedef struct log_system
{
	mutex_t mtx;
	struct     link_list *log_files; //创建的所有struct log
	thread_t   worker_thread;        //磁盘写线程
	thread_t   time_str_thread;      //更新日期字符串的线程
	int32_t    bytes;
	struct atomic_type *time_str;
	volatile   uint8_t terminate;

}*log_system_t;

log_system_t g_log_system;


static inline log_str_t new_log_str(uint8_t level,const char *str)
{
	uint32_t len = strlen(str) + sizeof(struct log_str) + 1;
	log_str_t s = (log_str_t)calloc(1,len);
	s->start_pos = 0;
	struct sys_time_str tmp;
	GetTimeStr(g_log_system->time_str,&tmp);
	snprintf(s->str,len,"%s%s%s\n",level_str[level],tmp.time_str,str);
	s->len = strlen(s->str);
	return s;
}

void log_write(log_t l,uint8_t level, const char *str)
{
	log_str_t s = new_log_str(level,str);
	mutex_lock(l->mtx);
	LINK_LIST_PUSH_BACK(l->log_queue,s);
	mutex_unlock(l->mtx);
}

static void destroy_log(log_t *l);
static void *worker_routine(void*);

static void *time_update_routine(void *arg)
{
	while(g_log_system->terminate == 0)
	{
		update_time_str(g_log_system->time_str);
		sleepms(5);
	}
	return NULL;
}


void init_log_system()
{
	assert(g_log_system == NULL);
	g_log_system = calloc(1,sizeof(*g_log_system));
	g_log_system->mtx = mutex_create();
	g_log_system->log_files = create_link_list();
	g_log_system->time_str = create_atomic_type(sizeof(struct sys_time_str));
	update_time_str(g_log_system->time_str);
	g_log_system->worker_thread = CREATE_THREAD_RUN(1,worker_routine,0);
	g_log_system->time_str_thread = CREATE_THREAD_RUN(1,time_update_routine,0);
	g_log_system->bytes = 0;
	g_log_system->terminate = 0;
}

void close_log_system()
{
	assert(g_log_system != NULL);
	g_log_system->terminate = 1;
	thread_join(g_log_system->worker_thread);
	thread_join(g_log_system->time_str_thread);
	while(!link_list_is_empty(g_log_system->log_files))
	{
		log_t l = LINK_LIST_POP(log_t,g_log_system->log_files);
		destroy_log(&l);
	}
	mutex_destroy(&g_log_system->mtx);
	destroy_link_list(&g_log_system->log_files);
	destroy_thread(&g_log_system->worker_thread);
	destroy_thread(&g_log_system->time_str_thread);
	destroy_atomic_type(&g_log_system->time_str);
	free(g_log_system);
	g_log_system = NULL;
}

static inline int32_t prepare_buffer(log_t l,int32_t beg_pos)
{
	uint32_t buffer_size = max_size - beg_pos;
	do{
		log_str_t s = LINK_LIST_POP(log_str_t,l->pending_write);
		if(s == NULL) break;
		if(s->len <= buffer_size)
		{
			memcpy(&l->write_buffer[beg_pos],&(s->str[s->start_pos]),s->len);
			buffer_size -= s->len;
			beg_pos += s->len;
			free(s);
		}
		else
		{
			memcpy(&l->write_buffer[beg_pos],&(s->str[s->start_pos]),buffer_size);
			s->start_pos += buffer_size;
			s->len -= buffer_size;
			beg_pos += buffer_size;
			//重新插入到pending_write头部
			LINK_LIST_PUSH_FRONT(l->pending_write,s);
			break;
		}
	}while(buffer_size);
	return beg_pos;
}

static void write_to_file(log_t l)
{
	mutex_lock(l->mtx);
	if(!link_list_is_empty(l->log_queue))
		link_list_swap(l->pending_write,l->log_queue);
	mutex_unlock(l->mtx);
	if(g_log_system->terminate)
	{
		log_str_t s = new_log_str(LOG_INFO,"close log sucessful\n");
		LINK_LIST_PUSH_BACK(l->pending_write,s);
	}
	int32_t start_pos = 0;
	if(!link_list_is_empty(l->pending_write))
	{
		do{
			int32_t buffer_size = prepare_buffer(l,start_pos);
			if(buffer_size <= 0)break;
#ifdef _WIN
			int32_t bytetransfer = write(l->file_descriptor,&(l->write_buffer[start_pos]),buffer_size);
#else
			int32_t bytetransfer = TEMP_FAILURE_RETRY(write(l->file_descriptor,&(l->write_buffer[start_pos]),buffer_size));
#endif
			if(bytetransfer <= 0){
				printf("write to log file error,errno: %d",errno);
				return;
			}else
			{
				l->file_size += bytetransfer;
				if(buffer_size == bytetransfer) 
					start_pos = 0;
				else 
					start_pos += bytetransfer;
			}
		}while(1);
	}
}

#ifdef _WIN
#define IWUSR _S_IWRITE
#define IRUSR _S_IREAD
#else
#define IWUSR S_IWUSR
#define IRUSR S_IRUSR
#endif

log_t create_log(const char *filename)
{
	log_t l = calloc(1,sizeof(*l));
	l->file_descriptor = open(filename,
		O_WRONLY|O_CREAT|O_APPEND,IWUSR | IRUSR
	);
	if(l->file_descriptor < 0)
	{
		free(l);
		l = NULL;
	}
	else
	{
		l->file_size = 0;
		l->mtx = mutex_create();
		l->log_queue = create_link_list();
		l->pending_write = create_link_list();
		mutex_lock(g_log_system->mtx);
		LINK_LIST_PUSH_BACK(g_log_system->log_files,l);
		mutex_unlock(g_log_system->mtx);
		log_write(l,LOG_INFO,"open log file");
	}
	return l;			
}

static void  destroy_log(log_t *l)
{
	mutex_lock((*l)->mtx);
	while(!link_list_is_empty((*l)->log_queue))
	{
		log_str_t s = LINK_LIST_POP(log_str_t,(*l)->log_queue);
		free(s);
	}
	mutex_unlock((*l)->mtx);
	close((*l)->file_descriptor);
	mutex_destroy(&(*l)->mtx);
	destroy_link_list(&(*l)->log_queue);
	destroy_link_list(&(*l)->pending_write);
	free(*l);
	*l = 0;
}


static inline void write_all_log_file()
{
	log_t l = (log_t)link_list_head(g_log_system->log_files);
	while(l)
	{
		write_to_file(l);
		l = (log_t)(((struct list_node*)l)->next);
	}
}

static void *worker_routine(void *arg)
{
	while(!g_log_system->terminate)
	{
		int32_t tick = GetSystemMs(); 
		write_all_log_file();
		tick = GetSystemMs() - tick;
		if(tick < 50)
			sleepms(50-tick);			
	}
	write_all_log_file();
	return NULL;
}