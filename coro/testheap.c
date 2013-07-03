#include <stdio.h>
#include "util/minheap.h"
#include "util/SysTime.h"
struct _timeout
{
	struct heapele _heapele;
	uint32_t timeout;
};

int8_t _less(struct heapele*l,struct heapele*r)
{
	return ((struct _timeout*)l)->timeout < ((struct _timeout*)r)->timeout;
}

int main()
{
	minheap_t m = minheap_create(512,_less);
	struct _timeout to1;
	to1.timeout = GetSystemMs() + 500;
	to1._heapele.index = 0;

	struct _timeout to2;
	to2.timeout = GetSystemMs() + 600;
	to2._heapele.index = 0;	
	
	minheap_insert(m,(struct heapele*)&to1);
	minheap_insert(m,(struct heapele*)&to2);
	while(1)
	{
		uint32_t tick = GetSystemMs();
		struct _timeout *t = (struct _timeout*)minheap_min(m);
		if(t && tick >= t->timeout)
		{
			minheap_popmin(m);
			printf("%x,%u\n",t,t->timeout);
			t->timeout = tick + 100;
			minheap_insert(m,(struct heapele*)t);
		}
		sleepms(50);
	}
	return 0;
}
