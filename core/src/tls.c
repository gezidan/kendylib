#include "tls.h"
#include "atomic.h"
#include "link_list.h"
#include "sync.h"
#include <stdlib.h>

static mutex_t       tls_mtx;
static pthread_key_t thread_key;
struct link_list     all_tls_struct = {0,NULL,NULL};
struct tls_struct
{
	LIST_NODE;
	void* slot[MAX_TLS_SLOT];
};
  
void init_tls()
{
	pthread_key_create(&thread_key,0);
	tls_mtx = mutex_create();
}

void clear_tls()
{
	struct tls_struct *st;
	while((st = (struct tls_struct*)link_list_pop(&all_tls_struct)) != NULL)
		free(st);
	pthread_key_delete(thread_key);
	mutex_destroy(&tls_mtx);
}


void *get_tls_data(uint32_t key)
{
	if(key >= MAX_TLS_SLOT)
		return NULL;
	struct tls_struct *tls_st = (struct tls_struct *)pthread_getspecific(thread_key);
	if(!tls_st)
		return NULL;
	return tls_st->slot[key];
}

void set_tls_data(uint32_t key,void *data)
{

	if(key >= MAX_TLS_SLOT)
		return;
	struct tls_struct *tls_st = (struct tls_struct *)pthread_getspecific(thread_key);
    if(!tls_st)
	{
		tls_st = (struct tls_struct *)malloc(sizeof(*tls_st));
		mutex_lock(tls_mtx);
		LINK_LIST_PUSH_BACK(&all_tls_struct,tls_st);
		mutex_unlock(tls_mtx);
	}
	tls_st->slot[key] = data;
}
