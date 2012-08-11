all:
	gcc -g -c data_struct.c dbtype.c global_table.c -I../include
	ar -rc dbcache.a *.o
dblist:../kendy.a dbcache.a testdblist.c
	gcc -g -o dblist testdblist.c dbcache.a ../kendy.a -I../include -lpthread -lrt -ltcmalloc
gtb:../kendy.a dbcache.a gtb.c
	gcc -g -o gtb gtb.c dbcache.a ../kendy.a -I../include -lpthread -lrt -ltcmalloc	
