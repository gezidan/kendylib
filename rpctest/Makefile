server:../kendy.a rpcserver.c
	gcc -O3 -g -o server rpcserver.c ../kendy.a -I../include -lpthread -lrt
client:../kendy.a ../coro/coro.a rpcclient.c
	gcc -O3 -g -o client rpcclient.c ../coro/coro.a ../kendy.a -I../include  -I../coro -lpthread -lrt		
