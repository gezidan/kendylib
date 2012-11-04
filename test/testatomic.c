#include <stdio.h>
#include "atomic.h"
#include "SysTime.h"

int main()
{
	atomic_32_t counter = 0;
	int32_t tick = GetSystemMs();
	int i = 0;
	for(i = 0; i < 100000000;++i)
		++counter;
	printf("%d\n",GetSystemMs() - tick);
	
	tick = GetSystemMs();
	counter = 0;
	for(i = 0; i < 100000000;++i)
		ATOMIC_INCREASE(&counter);
	printf("%d\n",GetSystemMs() - tick);
		
	return 0;
}
