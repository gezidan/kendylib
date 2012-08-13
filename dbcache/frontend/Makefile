all:
	gcc -g -c trans_table.c -I../../include -I../
test:testclient.c trans_table.o ../dbcache.a
	gcc -g -o test testclient.c trans_table.o ../dbcache.a ../../kendy.a -I../../include -I../ -lpthread -lrt -ltcmalloc
