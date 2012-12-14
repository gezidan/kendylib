server:kendy.a net.a testserver.c
	gcc -O3 -g -o server testserver.c net.a ../../coro/coro.a kendy.a -I../../include -I../ -lpthread -lrt -ltcmalloc
debug:kendy.a net.a testserver.c
	gcc -g -o server testserver.c net.a kendy.a -I../../include -I../ -lpthread -lrt	
