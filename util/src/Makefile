all:
	gcc -g -O3 -c *.c -I../ 
	ar -rc util.a *.o
	mv util.a ../../lib
	rm -f *.o
debug:
	gcc -g -c *.c -I../ -D _DEBUG
	ar -rc util.a *.o
	mv util.a ../../lib
	rm -f *.o
