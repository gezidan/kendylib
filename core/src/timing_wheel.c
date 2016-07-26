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
	int32_t    slot;
	TimingWheel_t timing_wheel;
	WheelItem_OnDestroy on_destroy;
};

struct TimingWheel
{
	uint32_t precision;
	uint32_t slot_size;
	uint32_t last_update;
	uint32_t current;
	WheelItem_t   slot[0];
};

void *GetUserData(WheelItem_t w)
{
	return w->ud;
}

WheelItem_t CreateWheelItem(void *ud,TimingWheel_callback callback,WheelItem_OnDestroy ondestroy)
{
	WheelItem_t wit = malloc(sizeof(*wit));
	if(!wit)
		return 0;
	wit->ud = ud;
	wit->callback = callback;
	wit->pre = wit->next = 0;
	wit->slot = -1;
	wit->on_destroy = ondestroy;
	return wit;
}

void DestroyWheelItem(WheelItem_t *wit)
{
	if((*wit)->on_destroy)
		(*wit)->on_destroy(*wit);
	free(*wit);
	*wit = 0;
}

TimingWheel_t CreateTimingWheel(uint32_t precision,uint32_t max)
{
	//init_system_time(10);
	uint32_t slot_size = max/precision;
	TimingWheel_t t = malloc(sizeof(*t) + (slot_size*sizeof(WheelItem_t)));
	if(!t)
		return 0;
	uint32_t i = 0;
	for(; i < slot_size; ++i)
		t->slot[i] = NULL;
	t->slot_size = slot_size;
	t->precision = precision;
	t->current = 0;
	t->last_update = GetSystemMs();
	return t;
}

void DestroyTimingWheel(TimingWheel_t *t)
{
	//��������ʣ��Ķ�ʱ��
	uint32_t i = 0;
	for( ; i < (*t)->slot_size; ++i)
	{
	   WheelItem_t cur = (*t)->slot[i];
	   while(cur)
       {
			WheelItem_t tmp = cur;		
			cur = cur->next;	
			DestroyWheelItem(&tmp);
      }
	}
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
	else
		item->next = item->pre = NULL;
	t->slot[slot] = item;
	item->slot = slot;
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
	item->timing_wheel = t;
	return 0;
}

//����slot�е������¼�
static void Active(TimingWheel_t t,uint32_t slot,uint32_t now)
{
   WheelItem_t head = t->slot[slot];
   t->slot[slot] = 0;
   while(head)
   {
		WheelItem_t cur = head;		
		head = head->next;
		cur->slot = 0;
		cur->pre = cur->next = 0;
		cur->timing_wheel = NULL;
		if(cur->callback)
			cur->callback(t,cur->ud,now);
		
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
		Active(t,(t->current+i)%t->slot_size,now);
	}
	t->current = (t->current+i)%t->slot_size;
	t->last_update = now;//+= (interval*t->precision);
	
	
	return 0;
}

void UnRegisterTimer(/*TimingWheel_t t,*/WheelItem_t wit)
{
	if(!wit)
		return;
	TimingWheel_t t = wit->timing_wheel;
	if(!t)
		return;
	if(wit->slot < 0 || wit->slot >= t->slot_size)
		return;
	WheelItem_t next = wit->next;
	WheelItem_t pre = wit->pre;		
	if(pre)
	{
		pre->next = next;
		if(next)
			next->pre = pre;
	}
	else
	{
		t->slot[wit->slot] = next;
		if(next)
			next->pre = NULL;
		else
			printf("empty\n");
	}
	
	wit->pre = wit->next = NULL;
	wit->slot = -1;
	printf("UnRegisterTimer\n");
}
