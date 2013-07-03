//dbcache后端基本接口设计

#ifdef USE_MYSQL
typedef MYSQL* DB_HANDLE;
#define INVAILD_DB_HANDLE NULL; 
#else
typedef int DB_HANDLE;
#endif
int      db_library_init();  //初始化DB库
void     db_library_end();   //释放DB库

DB_HANDLE db_connect(const char *ip,unsigned int port,const char *usr,const char *pass,const char *db);  //建立一个到数据库的连接
void      db_handle_close(DB_HANDLE*);//关闭,释放连接

//field的元数据
struct field_metadata
{
	const char    *name; //字段名
	unsigned int   max_length; //字段的最大长度(字节)
	unsigned short type;//字段类型
	basetype_t     default_value;//默认值
};

struct field_metadatas
{
	unsigned int size;
	struct field_metadata* _field_metadata;
};

typedef db_list_t result_set;

void     db_free_result_set(result_set*);//释放result_set

struct field_metadatas *db_get_table_metadata(DB_HANDLE,const char *table);//获取表中所有字段的元数据
result_set db_do_select(DB_HANDLE,const char *stmt);//执行select语句
int      db_do_update(DB_HANDLE,const char *stmt);//执行update语句
int      db_do_delete(DB_HANDLE,const char *stmt);//执行delete语句

