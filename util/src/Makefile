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
all-mingw:
	gcc -g -O3 -c *.c -I../  -D _MINGW_ -march=i686
	ar -rc util.a *.o
	mv util.a ../../lib
	rm -f *.o
debug-mingw:
	gcc -g -c *.c -I../ -D _DEBUG -D _MINGW_ -march=i686
	ar -rc util.a *.o
	mv util.a ../../lib
	rm -f *.o
