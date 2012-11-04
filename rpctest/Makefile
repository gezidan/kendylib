server:kendy.a rpcserver.c
	gcc -O3 -g -o server rpcserver.c kendy.a -I../include -lpthread -lrt -ltcmalloc
client:kendy.a ../coro/coro.a rpcclient.c
	gcc -O3 -g -o client rpcclient.c ../coro/coro.a kendy.a -I../include  -I../coro -lpthread -lrt -ltcmalloc
syncall:.kendy.a ../coro/coro.a synccall.c
	gcc -O3 -g -o syncall synccall.c ../coro/coro.a kendy.a -I../include  -I../coro -lpthread -lrt -ltcmalloc		
