#ifndef _SYSTIME_H
#define _SYSTIME_H
#include <stdint.h>
#include <time.h>
#include <sys/time.h> 
#include <unistd.h>
#include "common.h"
static inline uint32_t GetSystemMs()
{
	//struct timespec now;
    //clock_gettime(CLOCK_MONOTONIC, &now);
	//return now.tv_sec*1000 + now.tv_nsec/(1000*1000);
#ifdef _WIN
	return GetTickCount();
#else
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec*1000+now.tv_usec/1000;
#endif
	
}

static inline void sleepms(uint32_t ms)
{
	usleep(ms*1000);
}

static inline char *GetCurrentTimeStr(char *buf)
{
	struct tm _tm;
	time_t _now = time(NULL);
	localtime_r(&_now, &_tm);
	snprintf(buf,64,"[%04d-%02d-%02d %02d:%02d:%02d]",_tm.tm_year+1900,_tm.tm_mon+1,_tm.tm_mday,_tm.tm_hour,_tm.tm_min,_tm.tm_sec);
	return buf;
}

/*
struct system_time_mgr
{	
	volatile int32_t   current_index __attribute__((aligned(8)));	
	uint32_t ms[2];
	char     str[2][64];
	uint32_t sleep_time;	
};



extern struct system_time_mgr *stm;

extern void init_system_time(uint32_t);

static inline uint32_t GetCurrentMs()
{
#ifdef _WIN
	return GetTickCount();
#else
	int32_t index = stm->current_index;
	return stm->ms[index];
#endif
}

static inline const char *GetCurrentTimeStr()
{
	int32_t index = stm->current_index;
	return stm->str[index];
}
*/

#endif
