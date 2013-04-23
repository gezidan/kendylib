timer:
	gcc -g -o test testtimer.c kendy.a -I../include -lrt
thread:
	gcc -g -o test testthread.c ../lib/util.a -I../ -lpthread
wheel:kendy.a testwheel.c
	gcc -g -o wheel testwheel.c kendy.a -I../include -lrt -lpthread
accept_connect:
	gcc -g -o test_connect test_connector.c network.a -I../include -lpthread -lrt
	gcc -g -o test_accept test_acceptor.c network.a -I../include -lpthread -lrt
server:../lib/util.a ../lib/net.a testserver.c
	gcc -O3 -g -o server testserver.c ../lib/net.a ../lib/util.a -I../ -lpthread -lrt
client:../lib/util.a ../lib/net.a testclient.c
	gcc -O3 -g -o client testclient.c ../lib/net.a ../lib/util.a -I../ -I../include -lpthread -lrt
objpool:kendy.a
	gcc -O3 -g -o objpool testobjpool.c kendy.a -I../include -lpthread -lrt -ltcmalloc
spin:testspinlock.c
	gcc -g -o spin testspinlock.c ../lib/util.a -I../ -lpthread
ut:testuthread.c
	gcc -g -O3 -o ut testuthread.c ../lib/util.a -I../ -lpthread
ut-debug:testuthread.c
	gcc -g -o ut testuthread.c ../lib/util.a -I../ -lpthread	 
log:kendy.a testlog.c
	gcc -g -O3 -o log testlog.c kendy.a -I../include -lpthread -lrt -ltcmalloc
systime:kendy.a testsystime.c
	gcc -g -O3 -o systime testsystime.c kendy.a -I../include -lpthread -lrt -ltcmalloc
hash:testhash.c
	gcc -g -O3 -o hash testhash.c ../lib/util.a -I../ -lpthread
testmq:kendy.a testmq.c
	gcc -g -O3 -o testmq testmq.c kendy.a -I../include -lpthread -lrt -ltcmalloc
syn_st:test_atomic_st.c
	gcc -g -O3 -o syn_st test_atomic_st.c ../lib/util.a -I../ -lpthread
list:test_list.c
	gcc -g -o list test_list.c ../lib/util.a -I../ -lpthread
map:testmap.c
	gcc -g -o map testmap.c ../lib/util.a -I../ -lpthread	
