all:
	gcc -g -c data_struct.c dbtype.c -I../include
	ar -rc dbcache.a *.o
dblist:../kendy.a dbcache.a testdblist.c
	gcc -g -o dblist testdblist.c dbcache.a ../kendy.a -I../include -lpthread -lrt -ltcmalloc
