all:
	gcc -g -O3 -c minheap.c cosche.c -I../include
	ar -rc coro.a *.o
testheap:
	gcc -g -o testheap testheap.c minheap.o ../kendy.a -I../include -lpthread -lrt -ltcmalloc
testcoro:testcoro.c coro.a
	gcc -g -O3 -o testcoro testcoro.c coro.a ../kendy.a -I../include -lpthread -lrt -ltcmalloc
