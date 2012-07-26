#include "allocator.h"
struct first_fit_chunk
{
	//状态1为保留,0为自由
	int32_t tag;               //当前块的状态
	uint32_t size;     //当前块的大小
	union{
		struct{
			struct first_fit_chunk *next;//下一个空闲块
			struct first_fit_chunk *pre; //前一个空闲块
		};
		uint8_t buf[1];
	};
};

struct first_fit_pool
{
	IMPLEMEMT(allocator);
	struct first_fit_chunk ava_head;
	uint32_t pool_size;
	uint8_t *begin;
	uint8_t *end;
	uint8_t buf[1];
};



#define FREE_TAG 0XFEDCBA98
#define USE_TAG  0X89ABCDEF
#define CHUNK_HEAD (sizeof(int32_t) + sizeof(uint32_t))
#define RESERVESIZE (sizeof(struct first_fit_chunk) + sizeof(int32_t))
