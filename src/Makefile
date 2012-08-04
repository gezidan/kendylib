all:
	gcc -g -c *.c -I../include
	ar -rc kendy.a *.o
	cp kendy.a ../../luanet
	mv kendy.a ../
	rm -f *.o
all-mt:
	gcc -g -c *.c -I../include -I../../ -D MT
	ar -rc network_mt.a *.o
	mv network_mt.a ../
	rm -f *.o
timer:
	gcc -g -c timer.c -I../include
	ar -rc timer.a *.o
	mv timer.a ../
	rm -f *.o
wheel:
	gcc -g -c timing_wheel.c SysTime.c -I../include
	ar -rc timer.a *.o
	mv timer.a ../
	rm -f *.o		
