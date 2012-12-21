all:
	gcc -g -c *.c -I../include					
	gcc -g -o maze testmaze.o AStar.o kendy.a -lpthread -lrt
	gcc -g -o 8 8puzzle.o AStar.o kendy.a -lpthread -lrt