all:
	gcc -g -O3 -c *.c -I../../../ -I../../../ -I../../ -I../../../netframework -I../../../coro
server2:cache_server2.o
	gcc -g -O3 -o server2 cache_server2.o db_protocal.o ../dbcache.a ../../../lib/net.a ../../../lib/util.a -lpthread -lrt -ltcmalloc	
client3:testclient3.o
	gcc -g -O3 -o client3 testclient3.o db_protocal.o ../dbcache.a ../../../netframework/netframe.a ../../../lib/net.a ../../../coro/coro.a ../../../lib/util.a -lpthread -lrt	-ltcmalloc		
