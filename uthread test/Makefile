all:
	rm test
	rm *.o
	gcc -g -c coro.c -I../include
	gcc -g -o test test.c coro.o ../kendy.a -I../include -lpthread -lrt
	
