#include <stdio.h>
#include <winsock2.h>
#include "mysql.h"
int main()
{
    MYSQL mysql;
    mysql_init(&mysql);
    if(!mysql_real_connect(&mysql, "localhost", "root", "802802", "test", 3306, NULL, 0))
    {
        printf("\nconnect error!");
    }
    else
    {
        printf("\nconnect success!");
    }
    mysql_close(&mysql);

    printf("finished\n");

    return 0;
}