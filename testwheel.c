#include <stdio.h>
#include "SysTime.h"
#include "timing_wheel.h"
#include "spinlock.h"

struct testWheelItem
{
	unsigned long    lastactive;
	WheelItem_t item;
	TimingWheel_t    tw;
	int to;
};


void  call_back2(void *ud)
{
	unsigned long now = GetSystemMs();
	struct testWheelItem *tw = (struct testWheelItem *)ud;
	printf("to: %d,difflast:%d\n",tw->to,abs(now - tw->lastactive-tw->to));
	tw->lastactive = now;
	RegisterTimer(tw->tw,tw->item,tw->to);
}


int main()
{
	
	TimingWheel_t tw = CreateTimingWheel(10,10000000);
	struct testWheelItem twi1,twi2,twi3,twi4,twi5;
	twi1.item = CreateWheelItem(&twi1,call_back2);
	twi2.item = CreateWheelItem(&twi2,call_back2);
	twi3.item = CreateWheelItem(&twi3,call_back2);
	twi4.item = CreateWheelItem(&twi4,call_back2);
	twi5.item = CreateWheelItem(&twi5,call_back2);
	twi1.tw = tw;twi1.to = 10;
	twi2.tw = tw;twi2.to = 10;
	twi3.tw = tw;twi3.to = 10;
	twi4.tw = tw;twi4.to = 10;
	twi5.tw = tw;twi5.to = 10;
	unsigned long now = GetSystemMs();
	
	twi1.lastactive = now;
	RegisterTimer(tw,twi1.item,30);
	/*
	twi2.lastactive = now;
	RegisterTimer(tw,twi2.item,10);
	
	twi3.lastactive = now;
	RegisterTimer(tw,twi3.item,10);

	twi4.lastactive = now;
	RegisterTimer(tw,twi4.item,10);
	
	twi5.lastactive = now;
	RegisterTimer(tw,twi5.item,10);		
	
	UnRegisterTimer(tw,twi5.item);
	UnRegisterTimer(tw,twi4.item);
	UnRegisterTimer(tw,twi3.item);
	UnRegisterTimer(tw,twi2.item);
	*/
	
	UnRegisterTimer(tw,twi1.item);	
	
/*	
	int i = 0;
	for( ; i < 1000;++i)
	{
		UpdateWheel(tw,GetSystemMs());
		sleepms(2*1000);
	}
*/	
	return 0;
}


