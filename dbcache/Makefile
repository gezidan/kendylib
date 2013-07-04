all:
	gcc -O3 -g -c *.c -I../
	ar -rc dbcache.a *.o
	mv dbcache.a ./frontend
	rm -f *.o
debug:
	gcc -g -c *.c -I../
	ar -rc dbcache.a *.o
	mv dbcache.a ./frontend
	rm -f *.o
all-mysql:
	gcc -g -c db_impl.c -I../mysql-connector/include -I.. -I. -D _WIN -march=i686 -D USE_MYSQL
mysql:test.c
	gcc -g -o test test.c db_impl.o -I../mysql-connector/include -L../mysql-connector/lib -lmysql -I.. -I. -D _WIN -march=i686 -D USE_MYSQL -lpthread		
