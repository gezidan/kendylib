server:testserver.c
	gcc -O3 -g -o server testserver.c ../netframe.a ../../lib/net.a ../../coro/coro.a ../../lib/util.a -I../../ -I../ -lpthread -lrt -ltcmalloc
debug:kendy.a net.a testserver.c
	gcc -g -o server testserver.c net.a kendy.a -I../../include -I../ -lpthread -lrt
testcoronet:testcoronet.c
	gcc -O3 -g -o testcoronet testcoronet.c ../../lib/netframe.a ../../lib/net.a ../../lib/coro.a ../../lib/util.a -I../ -I../../ -lpthread -lrt -ltcmalloc
	rm -f *.o
