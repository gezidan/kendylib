#include <stdio.h>
#include "KendyNet.h"
#include "Connector.h"

void test_on_connect(HANDLE sock,const char *ip,unsigned long port,void*ud)
{
	if(sock >= 0)
	{
		printf("connect %s success\n",ip);
	}
	else
	{
		printf("connect error\n");
	}
}


int main()
{
	InitHandleMgr();
	connector_t c = connector_create();
	int i = 0;
	for(; i < 20; ++i)
		connector_connect(c,"192.168.6.204",8010,test_on_connect,0,100);
	for(;;)
	{
		connector_run(c,50);
	}

	return 0;
}