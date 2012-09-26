all:
	gcc -O3 -g -c minheap.c co_sche.c -I../include
	ar -rc coro.a *.o
	rm -f *.o
testheap:
	gcc -g -o testheap testheap.c coro.a ../kendy.a -I../include -lpthread -lrt -ltcmalloc
	rm -f *.o
testcoro:testcoro.c coro.a
	gcc -O3 -g -o testcoro testcoro.c coro.a ../kendy.a -I../include -lpthread -lrt -ltcmalloc
	rm -f *.o
