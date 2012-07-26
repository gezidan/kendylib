#include <stdio.h>
#include <stdlib.h>

#include "timing_wheel.h"
#include "SysTime.h"

struct WheelItem
{
	WheelItem_t pre;
	WheelItem_t next;
	void *ud;
	TimingWheel_callback callback;
	WheelItem_t* slot;
};

struct TimingWheel
{
	uint32_t precision;
	uint32_t slot_size;
	uint32_t last_update;
	uint32_t current;
	WheelItem_t   slot[0];
};

WheelItem_t CreateWheelItem(void *ud,TimingWheel_callback callback)
{
	WheelItem_t wit = malloc(sizeof(*wit));
	if(!wit)
		return 0;
	wit->ud = ud;
	wit->callback = callback;
	wit->pre = wit->next = 0;
	wit->slot = 0;
	return wit;
}

void DestroyWheelItem(WheelItem_t *wit)
{
	free(*wit);
	*wit = 0;
}

TimingWheel_t CreateTimingWheel(uint32_t precision,uint32_t max)
{
	uint32_t slot_size = max/precision;
	TimingWheel_t t = malloc(sizeof(*t) + (slot_size*sizeof(WheelItem_t)));
	if(!t)
		return 0;
	t->slot_size = slot_size;
	t->precision = precision;
	t->current = 0;
	t->last_update = GetSystemMs();
	return t;
}

void DestroyTimingWheel(TimingWheel_t *t)
{
	free(*t);
	*t = 0;
}

inline static void Add(TimingWheel_t t,uint32_t slot,WheelItem_t item)
{
	if(t->slot[slot])
	{
		t->slot[slot]->pre = item;
		item->next = t->slot[slot];
	}
	t->slot[slot] = item;
	item->slot = &(t->slot[slot]);
}

int  RegisterTimer(TimingWheel_t t,WheelItem_t item,uint32_t timeout)
{
	if(timeout < t->precision)
		timeout = t->precision;
	uint32_t now = GetSystemMs();
	int n = (now + timeout - t->last_update - t->precision)/t->precision;
	if(n >= t->slot_size)
		return -1;
	n = (t->current + n)%t->slot_size;
	Add(t,n,item);
	return 0;
}

//激活slot中的所有事件
static void Active(TimingWheel_t t,uint32_t slot)
{
   WheelItem_t head = t->slot[slot];
   t->slot[slot] = 0;
   while(head)
   {
		WheelItem_t cur = head;		
		head = head->next;
		cur->slot = 0;
		cur->pre = cur->next = 0;
		if(cur->callback)
			cur->callback(cur->ud);
		
   }
}

int UpdateWheel(TimingWheel_t t,uint32_t now)
{
	uint32_t interval = now - t->last_update;
	if(interval < t->precision)
		return -1;
	interval = interval/t->precision;
	uint32_t i = 0;
	for( ; i < interval && i < t->slot_size; ++i)
	{
		Active(t,(t->current+i)%t->slot_size);
	}
	t->current = (t->current+i)%t->slot_size;
	t->last_update = now;//+= (interval*t->precision);
	
	
	return 0;
}

void UnRegisterTimer(TimingWheel_t t,WheelItem_t wit)
{
	/*
	if(!wit->wt || wit->wt != t)
		return;
	
	if(wit->pre == 0 && wit->next == 0)
		t->slot[wit->slot] = 0;
	else
	{
		WheelItem_t next = wit->next;
		WheelItem_t pre = wit->pre;
		
		
		
		//wit->pre->next = wit->next;
		//wit->next->pre = wit->pre;
		//if(wit->next->pre = 0)
		//	t->slot[wit->slot] = wit->next;
	}
	wit->wt = 0;
	*/
}
