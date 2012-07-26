all:
	gcc -g -O3 -o test test.c kendy.a -I./include -lpthread -lrt
timer:
	gcc -g -o test testtimer.c timer.a -I./include -lrt
thread:
	gcc -g -o test testthread.c network.a -I./include -lpthread
wheel:
	gcc -g -o test testwheel.c timer.a -I./include -lrt
accept_connect:
	gcc -g -o test_connect test_connector.c network.a -I./include -lpthread -lrt
	gcc -g -o test_accept test_acceptor.c network.a -I./include -lpthread -lrt
server:
	gcc -g -o server testserver.c network.a ../clib/clib.a  -I./include -I../ -lpthread -lrt
client:
	gcc -g -o client testclient.c network.a ../clib/clib.a -I./include -I../ -lpthread -lrt
atomic:
	gcc -g -o atomic testatomic.c network.a ../clib/clib.a -I./include -I../ -lpthread -lrt
log:kendy.a
	gcc -g -o log testlog.c kendy.a -I./include -lpthread -lrt -ltcmalloc
