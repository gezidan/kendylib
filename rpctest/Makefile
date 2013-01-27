server:rpcserver.c
	gcc -O3 -g -o server rpcserver.c ../lib/net.a ../lib/util.a -I../ -lpthread -lrt -ltcmalloc
client:rpcclient.c
	gcc -O3 -g -o client rpcclient.c ../coro/coro.a ../lib/net.a ../lib/util.a -I../  -I../coro -lpthread -lrt -ltcmalloc
syncall:synccall.c
	gcc -O3 -g -o syncall synccall.c ../coro/coro.a ../lib/net.a ../lib/util.a -I../  -I../coro -lpthread -lrt -ltcmalloc		
