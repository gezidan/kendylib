all:
	gcc -g -O3 -c *.c -I../../../include -I../../ -I../../../netframework
server:cache_server.o
	gcc -g -O3 -o server cache_server.o db_protocal.o ../dbcache.a net.a ../kendy.a -lpthread -lrt -ltcmalloc
client:testclient.o
	gcc -g -O3 -o client testclient.o db_protocal.o ../dbcache.a net.a ../kendy.a -lpthread -lrt -ltcmalloc
