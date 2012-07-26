#include <stdio.h>
#include "KendyNet.h"
#include "Acceptor.h"

void test_on_accept(HANDLE sock,void* ud)
{
	printf("cli:%d\n",sock);
}

int main()
{
	InitHandleMgr();
	acceptor_t a = create_acceptor("192.168.6.204",8010,test_on_accept,0);
	for(;;)
		acceptor_run(a,50);
	return 0;
}