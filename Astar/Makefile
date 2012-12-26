all:
	gcc -g -O3 -c *.c -I../include -I../					
	gcc -g -O3 -o maze testmaze.o AStar.o ../lib/util.a -lpthread  
	gcc -g -O3 -o 8 8puzzle.o AStar.o ../lib/util.a -lpthread 
