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
struct system_time_mgr
{	
	volatile int32_t   current_index __attribute__((aligned(8)));	
	time_t   sec[2];
	uint32_t ms[2];
	char     str[2][64];
	
	uint32_t sleep_time;	
};


extern struct system_time_mgr *stm;

static inline sleepms(uint32_t ms)
{
	usleep(ms*1000);
}

extern void init_system_time(uint32_t);

static inline time_t   GetCurrentSec()
{
	int32_t index = stm->current_index;
	return stm->sec[index];
}

static inline uint32_t GetCurrentMs()
{
	int32_t index = stm->current_index;
	return stm->ms[index];
}

static inline const char *GetCurrentTimeStr()
{
	int32_t index = stm->current_index;
	return stm->str[index];
}


#endif
