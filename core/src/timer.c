#include <stdio.h>
#include <stdlib.h>
#include <sys/timerfd.h> 
#include "SocketWrapper.h"
#include "timer.h"
#include "epoll.h"

struct TimerMgr
{
	int32_t epollfd;
	volatile int terminated;
	struct epoll_event events[MAX_TIMER];
};

struct Timer
{
	int32_t   fd;
	void *arg;//callback的第二个参数 
	timer_callback callback;
};


TimerMgr_t CreateTimerMgr()
{
	int32_t epollfd = TEMP_FAILURE_RETRY(epoll_create(MAX_TIMER));
	if(epollfd>=0)
	{
		TimerMgr_t t = malloc(sizeof(*t));
		t->epollfd = epollfd;
		memset(t->events,0,sizeof(t->events));
		return t;
	}
	return 0;
}

void DestroyTimerMgr(TimerMgr_t *t)
{
	close((*t)->epollfd);
	free(*t);
	*t = 0;
}

void TerminateTimerMgr(TimerMgr_t t)
{
	t->terminated = 1;
}

void RunTimerMgr(TimerMgr_t t,int once)
{
	t->terminated = 0;
	int64_t tmp;
	while(!t->terminated && !once)
	{
		int nfds = TEMP_FAILURE_RETRY(epoll_wait(t->epollfd,t->events,MAX_TIMER,100));
		if(nfds < 0)
		{
			t->terminated = 1;
			break;
		}
		int i;
		for(i = 0 ; i < nfds ; ++i)
		{
			Timer_t _timer = (Timer_t)t->events[i].data.ptr;		
			TEMP_FAILURE_RETRY(read(_timer->fd,&tmp,sizeof(tmp)));
			if(_timer->callback)
				_timer->callback(_timer,_timer->arg);
		}
	}
	t->terminated = 1;
}

int   AddTimer(TimerMgr_t t,Timer_t _timer)
{
	int32_t ret;
	struct epoll_event ev;	
	ev.data.ptr = _timer;
	ev.events = EV_IN | EV_OUT;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(t->epollfd,EPOLL_CTL_ADD,_timer->fd,&ev));
	if(ret != 0)
		return -1;
	return 0;
}

int   RemoveTimer(TimerMgr_t t,Timer_t _timer)
{
	int32_t ret;
	struct epoll_event ev;
	TEMP_FAILURE_RETRY(ret = epoll_ctl(t->epollfd,EPOLL_CTL_DEL,_timer->fd,&ev));
	if(ret != 0)
		return -1;
	return 0;
}

void DefaultInit(struct itimerspec *new_value,int32_t interval)
{	
    struct timespec now;
    clock_gettime(/*CLOCK_REALTIME*/CLOCK_MONOTONIC, &now);
	int32_t sec = interval/1000;
	int32_t ms = interval%1000;	
	int64_t nosec = (now.tv_sec + sec)*1000*1000*1000 + now.tv_nsec + ms*1000*1000;
	new_value->it_value.tv_sec = nosec/(1000*1000*1000);
	new_value->it_value.tv_nsec = nosec%(1000*1000*1000);
	new_value->it_interval.tv_sec = sec;
	new_value->it_interval.tv_nsec = ms*1000*1000;
}

Timer_t CreateTimer(struct itimerspec *spec,timer_callback callback,void *arg)
{
	int32_t fd = timerfd_create(/*CLOCK_REALTIME*/CLOCK_MONOTONIC,0);
	if(fd < 0)
		return 0;
    Timer_t t = malloc(sizeof(*t));
    if(!t)
	{
		close(fd);
		return 0;
	}
	t->callback = callback;
	t->fd = fd;
	t->arg = arg;
	timerfd_settime(fd,TFD_TIMER_ABSTIME,spec,0);
	return t;
}

void DestroyTimer(Timer_t *t)
{
	free(*t);
	*t = 0;
}
