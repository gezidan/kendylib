#include "interface_design.h"
#ifdef USE_MYSQL
static void print_error (DB_HANDLE conn, char *message)
{
	fprintf (stderr, "%s\n", message);
	if (conn != NULL)
	{
		fprintf (stderr, "Error %u (%s): %s\n",
			mysql_errno (conn), mysql_sqlstate (conn), mysql_error (conn));
	}
}

int db_library_init()
{
	if (mysql_library_init (0, NULL, NULL))
	{
		print_error (NULL, "mysql_library_init() failed");
		return -1;
	}
	return 0;
}

void db_library_end()
{
	mysql_library_end ();
}

DB_HANDLE db_connect(const char *ip,unsigned int port,const char *usr,const char *pass,const char *db)
{

	DB_HANDLE conn = mysql_init (NULL);
	if (conn == NULL)
	{
		print_error (NULL, "mysql_init() failed (probably out of memory)");
		return INVAILD_DB_HANDLE;
	}
	if(mysql_real_connect(conn,ip,usr,pass,db,port,NULL,0) == NULL)
	{
		print_error (conn, "mysql_real_connect() failed");
		mysql_close (conn);
		return INVAILD_DB_HANDLE;
	}
	return conn;
}

void db_handle_close(DB_HANDLE conn*)
{
	if(conn && *conn)
	{
		mysql_close(*conn);
		*conn = NULL;
	}
}


static struct result_set* create_result_set(unsigned int field_num)
{

}

static basetype_t make_defaule_value(MYSQL_FIELD *field)
{
	return NULL;
}

static result_set process_result_set (DB_HANDLE conn,MYSQL_RES *res_set)
{
	MYSQL_ROW     row;
	MYSQL_FIELD   **field;
	unsigned long field_num;
	unsigned int  i;
	mysql_field_seek (res_set, 0);
	field_num = mysql_num_fields (res_set);
	field = malloc(sizeof(MYSQL_FIELD*)*field_num);
	if(field == NULL)
		return NULL;
	struct result_set* result = create_result_set(field_num);
	if(result == NULL)
	{
		free(field);
		return NULL;
	}
	while ((row = mysql_fetch_row (res_set)) != NULL)
	{
		mysql_field_seek (res_set, 0);
		for (i = 0; i < mysql_num_fields (res_set); i++)
		{			
			if (row[i] == NULL) 
				printf (" %s ","NULL");
			else if (IS_NUM (field[i]->type)) 
				printf (" %s ",row[i]);
			else              
				printf (" %s ",row[i]);
		}
		fputc ('\n', stdout);
	}
	free(field);
	return result;
}

result_set db_do_select(DB_HANDLE conn,const char *stmt_str)
{
	MYSQL_RES *res_set;
	if (mysql_query(conn, stmt_str) != 0)
	{
		print_error(conn, "Could not execute statement");
		return NULL;
	}
	res_set = mysql_store_result(conn);
	if(res_set)
	{
		result_set result = process_result_set(conn, res_set);
		mysql_free_result(res_set);
		return result;
	}
	return NULL;
}

struct field_metadatas *db_get_table_metadata(DB_HANDLE conn,const char *table)
{
	MYSQL_RES *res_set;
	res_set = mysql_list_fields(conn,table,NULL);
	if(res_set)
	{
		struct field_metadatas* metadatas = malloc(sizeof(struct field_metadatas));
		MYSQL_FIELD   **field;
		unsigned int  i;
		mysql_field_seek(res_set, 0);
		metadatas->size = mysql_num_fields(res_set);
		metadatas->_field_metadata = malloc(sizeof(struct field_metadata)*field_size);
		field = malloc(sizeof(MYSQL_FIELD*)*field_size);
		for (i = 0; i < metadatas->size; i++)
		{
			field[i] = mysql_fetch_field (res_set);
			metadatas->_field_metadata[i].name = field[i]->name;
			metadatas->_field_metadata[i].type = field[i]->type;
			metadatas->_field_metadata[i].max_length = field[i]->max_length;
			metadatas->_field_metadata[i].default_value = make_defaule_value(field[i]);
		}
		free(field);
		mysql_free_result (res_set);
		return metadatas;
	}
	print_error(conn, "Could not execute statement");
	return NULL;
}
#else

#endif

