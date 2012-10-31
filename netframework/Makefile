all:
	gcc -g -O3 -c *.c -I../include 
	ar -rc net.a *.o
	rm -f *.o
debug:
	gcc -g -c *.c -I../include -D _DEBUG
	ar -rc net.a *.o
	rm -f *.o	
