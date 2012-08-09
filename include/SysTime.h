#ifndef _SYSTIME_H
#define _SYSTIME_H
#include <sys/timerfd.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h> 
static inline uint32_t GetSystemMs()
{
	//struct timespec now;
    //clock_gettime(CLOCK_MONOTONIC, &now);
	//return now.tv_sec*1000 + now.tv_nsec/(1000*1000);
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec*1000+now.tv_usec/1000;
	
}

static inline sleepms(uint32_t ms)
{
	usleep(ms*1000);
}

extern void init_system_time(uint32_t);
extern inline time_t   GetCurrentSec();
extern inline uint32_t GetCurrentMs();
extern inline const char *GetCurrentTimeStr();

#endif
