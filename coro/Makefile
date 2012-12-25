all:
	gcc -O3 -g -c co_sche.c -I../
	ar -rc coro.a *.o
	rm -f *.o
debug:
	gcc -g -c co_sche.c -I../
	ar -rc coro.a *.o
	rm -f *.o	
testheap:
	gcc -g -o testheap testheap.c coro.a ../lib/util.a ../lib/net.a -I../ -lpthread -lrt
	rm -f *.o
testcoro:testcoro.c coro.a
	gcc -O3 -g -o testcoro testcoro.c coro.a ../lib/util.a ../lib/net.a -I../ -lpthread -lrt
	rm -f *.o
