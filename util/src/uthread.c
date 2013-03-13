#include "uthread.h"
#include <stdlib.h>
#include <pthread.h>
#include "link_list.h"
#define _X64
#ifdef _X64
#include "uthread_64.h"
#else
#include "uthread_32.h"
#endif