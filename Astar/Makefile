all:
	gcc -g -O3 -c *.c -I../include					
	gcc -g -O3 -o maze testmaze.o AStar.o kendy.a -lpthread -lrt -ltcmalloc
	gcc -g -O3 -o 8 8puzzle.o AStar.o kendy.a -lpthread -lrt -ltcmalloc
