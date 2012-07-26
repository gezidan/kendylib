#include "log.h"
#include <stdio.h>
#include "atomic.h"
#include "wpacket.h"
//#include "packet_allocator.h"
#include <stdlib.h>
#include "SysTime.h"
#include <string.h>
#include "block_obj_allocator.h"
//#include "mem_allocator.h"
//#include "fix_obj_pool.h"
uint32_t GetSize_of_pow2(uint32_t size);
uint8_t GetK(uint32_t size);





void test1(char **tmp)
{
	{
		allocator_t a = (allocator_t)create_block_obj_allocator(16);
		int j = 0;
		for(;j<10;++j)
		{
			uint32_t tick = GetSystemMs();
			int i = 0;
			for( ; i < 10000000; ++i)
			{
				tmp[i] = ALLOC(a,16);
			}
			printf("block_obj_allocator finish:%d\n",GetSystemMs()-tick);
			i = 0;
			for(; i < 10000000;++i)
				FREE(a,tmp[i]);			
	    }
	    DESTROY(&a);
	}
	{
		int j = 0;
		for(;j<10;++j)
		{		
			uint32_t tick = GetSystemMs();
			int i = 0;
			for( ; i < 10000000; ++i)
			{
				tmp[i] = malloc(16);
			}
			printf("tcmalloc finish:%d\n",GetSystemMs()-tick);			
			i = 0;
			for(; i < 10000000;++i)
				free(tmp[i]);

	    }
	}	
}

void test2(char **tmp)
{
	{
		allocator_t a = (allocator_t)create_block_obj_allocator(16);
		int j = 0;
		for(;j<10;++j)
		{
			uint32_t tick = GetSystemMs();
			int i = 0;
			for( ; i < 10000000; ++i)
			{
				tmp[i] = ALLOC(a,16);
			}
			i = 0;
			for(; i < 10000000;++i)
				FREE(a,tmp[i]);	
			printf("block_obj_allocator finish:%d\n",GetSystemMs()-tick);		
	    }
	    DESTROY(&a);
	}
	{
		int j = 0;
		for(;j<10;++j)
		{		
			uint32_t tick = GetSystemMs();
			int i = 0;
			for( ; i < 10000000; ++i)
			{
				tmp[i] = malloc(16);
			}
					
			i = 0;
			for(; i < 10000000;++i)
				free(tmp[i]);
			printf("tcmalloc finish:%d\n",GetSystemMs()-tick);	
	    }
	}	
}

void test3(char **tmp)
{
	{
		allocator_t a = (allocator_t)create_block_obj_allocator(16);
		int j = 0;
		for(;j<10;++j)
		{
			uint32_t tick = GetSystemMs();
			int i = 0;
			for( ; i < 10000000; ++i)
			{
				tmp[i] = ALLOC(a,16);
				if((i+1)%100000 == 0)
				{
					int k = (i+1)-100000;
					for(; k < i+1;++k)
						FREE(a,tmp[k]);
				}
			}
			printf("block_obj_allocator finish:%d\n",GetSystemMs()-tick);		
	    }
	    DESTROY(&a);
	}
	{
		int j = 0;
		for(;j<10;++j)
		{		
			uint32_t tick = GetSystemMs();
			int i = 0;
			for( ; i < 10000000; ++i)
			{
				tmp[i] = malloc(16);
				if((i+1)%100000 == 0)
				{
					int k = (i+1)-100000;
					for(; k < i+1;++k)
						free(tmp[k]);
				}
			}
			printf("tcmalloc finish:%d\n",GetSystemMs()-tick);	
	    }
	}	
}





#include "spinlock.h"
int main()
{	
	
	
	char **tmp = calloc(1,sizeof(char*)*10000000);
	test1(tmp);
	printf("test1 finish------------\n");
	test2(tmp);
	printf("test2 finish------------\n");
	test3(tmp);	
	printf("test3 finish------------\n");
    free(tmp);
	getchar();
	

			
    
	
	//char *tmp[48];
	//int i=0;
	//for(; i < 48;++i)
	//	tmp[i] = ALLOC(a,65532);
	//i = 0;	
	//for(; i < 48;++i)
	//	FREE(a,tmp[i]);
	//DESTROY(&a);
	//init_log_system();
	//log_t l = create_log("t_log.log");
	/*	
	 {   
		allocator_t w_pool = (allocator_t)create_wpacket_allocator(8192,0);
		allocator_t  b_pool = (allocator_t)create_buffer_allocator(0);
		int j = 0;
		for( ; j < 10;++j)
		{
			uint32_t tick = GetSystemMs();
			int i = 0;
			for(;i<1000000;++i)
			{
				wpacket_t wpk1 = wpacket_create(b_pool,w_pool,1,0);
				wpacket_t wpk2 = wpacket_create(b_pool,w_pool,17,0);
				wpacket_t wpk3 = wpacket_create(b_pool,w_pool,33,0);
				wpacket_t wpk4 = wpacket_create(b_pool,w_pool,65,0);
				wpacket_t wpk5 = wpacket_create(b_pool,w_pool,129,0);
				wpacket_t wpk6 = wpacket_create(b_pool,w_pool,257,0);
				wpacket_t wpk7 = wpacket_create(b_pool,w_pool,513,0);
				wpacket_t wpk8 = wpacket_create(b_pool,w_pool,1025,0);
				wpacket_t wpk9 = wpacket_create(b_pool,w_pool,2049,0);
				wpacket_t wpk10 = wpacket_create(b_pool,w_pool,4097,0);
				wpacket_destroy(&wpk1);
				wpacket_destroy(&wpk2);
				wpacket_destroy(&wpk3);
				wpacket_destroy(&wpk4);
				wpacket_destroy(&wpk5);
				wpacket_destroy(&wpk6);	
				wpacket_destroy(&wpk7);	
				wpacket_destroy(&wpk8);
				wpacket_destroy(&wpk9);																										
				wpacket_destroy(&wpk10);																								
				//wpacket_write_string(wpk,"hello kenny");
				
			}	
			printf("pool_alloc write finish:%d\n",GetSystemMs()-tick);
		}
	 }   
	
	{
		int j = 0; 
		for( ; j < 10;++j)
		{
			uint32_t tick = GetSystemMs();
			int i = 0;
			for(;i<1000000;++i)
			{
				wpacket_t wpk1 = wpacket_create(NULL,NULL,1,0);
				wpacket_t wpk2 = wpacket_create(NULL,NULL,17,0);
				wpacket_t wpk3 = wpacket_create(NULL,NULL,33,0);
				wpacket_t wpk4 = wpacket_create(NULL,NULL,65,0);
				wpacket_t wpk5 = wpacket_create(NULL,NULL,129,0);
				wpacket_t wpk6 = wpacket_create(NULL,NULL,257,0);
				wpacket_t wpk7 = wpacket_create(NULL,NULL,513,0);
				wpacket_t wpk8 = wpacket_create(NULL,NULL,1025,0);
				wpacket_t wpk9 = wpacket_create(NULL,NULL,2049,0);
				wpacket_t wpk10 = wpacket_create(NULL,NULL,4097,0);
				wpacket_destroy(&wpk1);
				wpacket_destroy(&wpk2);
				wpacket_destroy(&wpk3);
				wpacket_destroy(&wpk4);
				wpacket_destroy(&wpk5);
				wpacket_destroy(&wpk6);	
				wpacket_destroy(&wpk7);	
				wpacket_destroy(&wpk8);
				wpacket_destroy(&wpk9);																										
				wpacket_destroy(&wpk10);	
			}	
			printf("tcmalloc write finish:%d\n",GetSystemMs()-tick);
		}
	}*/ 
	//getchar();
	//close_log_system();
 
	return 0;
}
