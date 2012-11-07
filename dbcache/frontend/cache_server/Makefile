all:
	gcc -g -c *.c -I../../../include -I../../ -I../../../netframework
server:cache_server.o
	gcc -g -o server cache_server.o db_protocal.o ../dbcache.a net.a ../kendy.a -lpthread -lrt
client:testclient.o
	gcc -g -o client testclient.o db_protocal.o ../dbcache.a net.a ../kendy.a -lpthread -lrt
client2:testclient.o
	gcc -g -o client2 testclient2.o db_protocal.o ../dbcache.a net.a ../kendy.a -lpthread -lrt	
