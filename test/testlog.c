#include <stdio.h>
#include "core/log.h"
#include "core/SysTime.h"
#include <stdlib.h>

int main()
{   
   init_log_system(NULL);
   log_t l = create_log(NULL,"log.txt");
   if(l)
	log_write(l,LOG_INFO,"hello world");
   getchar();
   close_log_system(); 			
};
