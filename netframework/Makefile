all:
	gcc -g -O3 -c *.c -I../
	ar -rc netframe.a *.o
	rm -f *.o
debug:
	gcc -g -c *.c -I../ -D _DEBUG
	ar -rc netframe.a *.o
	rm -f *.o
