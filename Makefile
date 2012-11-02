all:
	gcc -g -O3 -o test test.c kendy.a -I./include -lpthread -lrt
timer:
	gcc -g -o test testtimer.c kendy.a -I./include -lrt
thread:
	gcc -g -o test testthread.c kendy.a -I./include -lpthread
wheel:kendy.a testwheel.c
	gcc -g -o wheel testwheel.c kendy.a -I./include -lrt -lpthread
accept_connect:
	gcc -g -o test_connect test_connector.c network.a -I./include -lpthread -lrt
	gcc -g -o test_accept test_acceptor.c network.a -I./include -lpthread -lrt
server:kendy.a testserver.c
	gcc -O3 -g -o server testserver.c kendy.a -I./include -lpthread -lrt -ltcmalloc
client:kendy.a testclient.c
	gcc -O3 -g -o client testclient.c kendy.a -I./include -lpthread -lrt -ltcmalloc
atomic:
	gcc -g -o atomic testatomic.c kendy.a ../clib/clib.a -I./include -I../ -lpthread -lrt
objpool:kendy.a
	gcc -O3 -g -o objpool testobjpool.c kendy.a -I./include -lpthread -lrt -ltcmalloc
spin:kendy.a
	gcc -g -o spin testspinlock.c kendy.a -I./include -lpthread -lrt
ut:kendy.a testuthread.c
	gcc -g -O3 -o ut testuthread.c kendy.a -I./include -lpthread -lrt -ltcmalloc
ut-debug:kendy.a testuthread.c
	gcc -g -o ut testuthread.c kendy.a -I./include -lpthread -lrt -ltcmalloc	 
log:kendy.a testlog.c
	gcc -g -O3 -o log testlog.c kendy.a -I./include -lpthread -lrt -ltcmalloc
systime:kendy.a testsystime.c
	gcc -g -O3 -o systime testsystime.c kendy.a -I./include -lpthread -lrt -ltcmalloc
hash:kendy.a testhash.c
	gcc -g -O3 -o hash testhash.c kendy.a -I./include -lpthread -lrt -ltcmalloc
testmq:kendy.a testmq.c
	gcc -g -O3 -o testmq testmq.c kendy.a -I./include -lpthread -lrt -ltcmalloc							
