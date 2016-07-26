#include <stdio.h>
#include <stdlib.h>
#include <sys/timerfd.h> 
#include "timer.h"


static int total = 0;

void test_callback(Timer_t t,void *arg)
{
	struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
	printf("%d,%ld\n",now.tv_sec,now.tv_nsec);
	++total;
	TimerMgr_t tmgr = (TimerMgr_t)arg;
	if(total == 20)
	{
		RemoveTimer(tmgr,t);
		DestroyTimer(&t);
		TerminateTimerMgr(tmgr);
	}
}

int main()
{
	TimerMgr_t t = CreateTimerMgr();
	Timer_t _timer = DEFAULT_TIMER(500,test_callback,(void*)t);
	AddTimer(t,_timer);
	RunTimerMgr(t);
	DestroyTimerMgr(&t);
	return 0;
}