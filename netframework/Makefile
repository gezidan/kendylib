all:
	gcc -g -O3 -c *.c -I../include
	ar -rc net.a *.o
	rm -f *.o
	cp net.a ../dbcache/frontend/cache_server
	mv net.a ./test
debug:
	gcc -g -c *.c -I../include -D _DEBUG
	ar -rc net.a *.o
	rm -f *.o
	cp net.a ../dbcache/frontend/cache_server
	mv net.a ./test	
