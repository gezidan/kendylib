#include <stdio.h>
#include "log.h"
#include "SysTime.h"
#include "wpacket.h"
#include <stdlib.h>
uint32_t log_count = 0;
uint32_t GetSize_of_pow2(uint32_t size);

int main()
{   
   init_system_time(10);
   init_log_system();
   log_t l = create_log("log.log");
   int i = 0; 
   uint32_t tick = GetSystemMs();
   for( ;i < 10000000; ++i)
   {
	   //GetCurrentTimeStr();
	   //buffer_t b = buffer_create_and_acquire(0,NULL,16);
	   //buffer_t c = buffer_acquire(NULL,b);
	   //buffer_release(&b);
	   //buffer_release(&c);
	   //int size = GetSize_of_pow2(16);
	   //wpacket_t w = wpacket_create(0,NULL,4096,0);
	   //wpacket_destroy(&w);
	   //void *ptr = malloc(512);
	   //free(ptr);
	   	log_write(l,"hello kenny",0);
		/*log_write(l,"hello kennyhello kennyhello kennyhello kennyhello kennyhello kenny"
		 "hello kennyhello kennyhello kennyhello kennyhello kennyhello kennyhello kenny"
		 "hello kennyhello kennyhello kennyhello kennyhello kennyhello kennyhello kenny"
		 "hello kennyhello kennyhello kennyhello kennyhello kennyhello kennyhello kenny"
		 "hello kennyhello kennyhello kennyhello kennyhello kennyhello kennyhello kenny"
		 "hello kennyhello kennyhello kennyhello kennyhello kennyhello kennyhello kenny"
		 "hello kennyhello kennyvhello kennyhello kennyhello kennyhello kennyhello kenny",0);*/	   
   }
   printf("write finish:%u\n",GetSystemMs()-tick);
   getchar();
   close_log_system(); 			
};
