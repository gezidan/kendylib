all:
	gcc -g -O3 -c *.c -I../ -I../../ 
	ar -rc net.a *.o
	mv net.a ../../lib
	rm -f *.o
debug:
	gcc -g -c *.c -I../ -I../../ -D _DEBUG -Wall
	ar -rc net.a *.o
	mv net.a ../../lib
	rm -f *.o
