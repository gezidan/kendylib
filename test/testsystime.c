#include "SysTime.h"
#include <stdio.h>

int main()
{
   init_system_time(10);
   int i = 0; 
   uint32_t tick = GetSystemMs();
   for( ;i < 10000000; ++i)
   {
	   GetSystemMs();
   }
   printf("GetSystemMs 1000W finish:%u\n",GetSystemMs()-tick);
   i = 0; 
   tick = GetSystemMs();
   for( ;i < 10000000; ++i)
   {
	   GetCurrentMs();
   }
   printf("GetCurrentMs 1000W finish:%u\n",GetSystemMs()-tick);   
   return 0;
}
