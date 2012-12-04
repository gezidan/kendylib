#include "coronet.h"
#include "msg.h"

coronet_t coronet_create()
{
	coronet_t coron = calloc(1,sizeof(*coron));
	coron->last_check_timer = GetCurrentMs();
	return coron;
}

struct per_thread_struct;
struct per_thread_struct* mq_push_local(mq_t m,struct list_node *msg);

static inline timeout_callback(TimingWheel_t t,void *ud,uint32_t now)
{
	struct coronet_timer *_timer = (struct coronet_timer*)ud;
	msg_t msg = create_msg(_timer,MSG_USER_TIMER_TIMEOUT);
	netservice_t nets = _timer->coron->nets;
	mq_push_local(nets->mq_out,msg);
}

void coronet_init_net(coronet_t coron,on_packet _on_packet,on_new_connection _on_new_connection,
	on_connection_disconnect _on_connection_disconnect,on_send_block _on_send_block)
{
	init_net_service();
	coron->nets = create_net_service(1);
	coron->msgl = create_msg_loop(_on_packet,_on_new_connection,_on_connection_disconnect,_on_send_block);
	CreateTimingWheel();
}

void coronet_init_coro(coronet_t coron,int32_t max_coro,int32_t stack_size,void (*idel)(void*),void *idel_arg)
{
	coron->coro_sche = sche_create(max_coro,stack_size,idel,idel_arg);
}

void coronet_destroy(coronet_t *_coron)
{
	coronet_t coron = *_coron;
	destroy_net_service(&coron->nets);
	destroy_msg_loop(&coron->msgl);
	sche_destroy(&coron->coro_sche);
	free(coron);
	*_coron = NULL;
}

int32_t _coronet_add_timer(coronet_t coron,struct coronet_timer *_timer)
{
	int32_t ret = 0;
	if(timeout < 1000)
		ret = RegisterTimer(coron->timer_ms,_timer->wheel_item,_timer->timeout);
	else if(timeout < 1000 * 60)
		ret = RegisterTimer(coron->timer_s,_timer->wheel_item,_timer->timeout);
	else	
		ret = RegisterTimer(coron->timer_m,_timer->wheel_item,_timer->timeout);
		
	if(ret !=0)
	{
		DestroyWheelItem(&_timer->wheel_item);
		free(_timer);
	}
	return ret;
}


static inline int32_t rpk_check(coro_t co_wakeup,rpacket_t r)
{
	/*这里必须做些判断
	* 1)co_wakeup确实在等待事件
	* 2)co_wakeup等待的事件就是当前rpacket_t包含的事件
	*/
	return 1;
}

void process_rpc_return(rpacket_t r)
{
	coro_t co = get_current_coro();
	coro_t co_wakeup = (coro_t)rpacket_read_uint32(r);
	if(rpk_check(co_wakeup,r))
	{
		co_wakeup->rpc_response = r;
		co_wakeup->_goback = co;
		//直接跳过去执行co_wakeup
		set_current_coro(co_wakeup);
		uthread_switch(co->ut,co_wakeup->ut,co);
	}		
}

int32_t coronet_add_timer(coronet_t coron,coronet_timer_callback callback,void *ud,uint32_t timeout)
{
	struct coronet_timer *_timer = calloc(1,sizeof(*_timer));
	_timer->wheel_item = CreateWheelItem(_timer,timeout_callback);
	_timer->coron = coron;
	_timer->ud = ud;
	_timer->timeout = timeout;
	return _coronet_add_timer(coron,_timer);
}

static inline void coronet_check_user_timer(coronet_t coron)
{
	uint32_t now = GetCurrentMs();
	if(now - coron->last_check_timer >= 50)
	{
		UpdateWheel(coron->timer_ms,now);
		UpdateWheel(coron->timer_s,now);
		UpdateWheel(coron->timer_m,now);
		coron->last_check_timer = now;
	}
}

coro_t _sche_next_1(sche_t s,coro_t co);
void check_time_out(sche_t s,uint32_t now);

void peek_msg(coronet_t coron,uint32_t ms)
{
	assert(coron);
	assert(coron->nets);
	assert(coron->msgl);
	assert(coron->coro_sche);
	coro_t co = get_current_coro();

	//首先检查是否有超时的coro,如果有唤醒,被唤醒后的coro会被投入到active_list_1
	check_time_out(coron->coro_sche,GetCurrentMs());
	//查看active_list_1中是否有coro等待执行,如果有,优先先执行coro
	_sche_next_1(coron->coro_sche,co);
	//检查用户定时器,如果有超时事件会触发一个消息并投递到消息队列中,后面的msg_loop_once会提取并执行
	coronet_check_user_timer(coron);
	//等待消息的到来
	msg_loop_once(coron->msgl,coron->nets,ms);	
}


HANDLE coronet_add_listener(coronet_t coron,const char *ip,uint32_t port)
{
	return add_listener(coron->nets,ip,port);
}

void coronet_rem_listener(coronet_t coron,HANDLE h)
{
	rem_listener(coron->nets,h);
}

int32_t coronet_connect(coronet_t coron,const char *ip,uint32_t port)
{
	return net_connect(coron->nets,ip,port);
}