#include "db.h"

int main()
{
	db_library_init();
	DB_HANDLE h = db_connect(
		"localhost", 3306,"root", "802802", "test"
		);
	//db_do_select(h,"SELECT * FROM test.test;");
	struct field_metadatas *mt = db_get_table_metadata(h,"test");
	if(mt)
	{
		int i = 0;
		for(; i < mt->size; ++i)
		{
			printf("%s,%d\n",mt->_field_metadata[i].name,mt->_field_metadata[i].max_length);
		}
	}
	db_library_end();
	return 0;
}