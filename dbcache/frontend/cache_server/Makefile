all:
	gcc -g -O3 -c *.c -I../../../include -I../../ -I../../../netframework -I../../../coro
server:cache_server.o
	gcc -g -O3 -o server cache_server.o db_protocal.o ../dbcache.a net.a ../kendy.a -lpthread -lrt -ltcmalloc
client:testclient.o
	gcc -g -o client testclient.o db_protocal.o ../dbcache.a net.a ../kendy.a -lpthread -lrt
client2:testclient2.o
	gcc -g -o client2 testclient2.o db_protocal.o ../dbcache.a net.a ../kendy.a -lpthread -lrt
client3:testclient3.o
	gcc -g -O3 -o client3 testclient3.o db_protocal.o ../dbcache.a net.a ../../../coro/coro.a ../kendy.a -lpthread -lrt	-ltcmalloc		
