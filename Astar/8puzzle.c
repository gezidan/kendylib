/*8码问题测试用例*/
#include "AStar.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "link_list.h"
#include "common_hash_function.h"
#include <stdlib.h>
#include <string.h>

struct _8puzzle_map;
struct _8puzzle_node
{
	struct map_node _base;
	list_node _lnode;
	struct _8puzzle_map *_map;
	int puzzle[3][3];
	int zero_x;
	int zero_y;
};

int direction[4][2] = {
	{0,-1},//上
	{0,1},//下
	{-1,0},//左
	{1,0},//右
};

static inline int32_t _hash_key_eq_(void *l,void *r)
{
	if(*(uint64_t*)r == *(uint64_t*)l)
		return 0;
	return -1;
}

static inline uint64_t _hash_func_(void* key)
{
	return burtle_hash(key,sizeof(uint64_t),1);
}

static inline uint64_t puzzle_value(int p[3][3])
{
	uint64_t pv = 0;
	int c = 100000000;
	int i = 0;
	int j = 0;
	for( ; i < 3; ++i)
	{
		for(j = 0;j < 3;++j)
		{
			pv += p[i][j]*c;
			c/=10;
		}
	}
	return pv;
}

struct _8puzzle_map
{	
	struct link_list *mnodes;
	hash_map_t puzzle_2_mnode;
};

struct _8puzzle_node* getnode_by_pv(struct _8puzzle_map *pmap,int p[3][3])
{
	hash_map_t h = pmap->puzzle_2_mnode;
	uint64_t pv = puzzle_value(p);
	struct _8puzzle_node* pnode = NULL;
	hash_map_iter it = HASH_MAP_FIND(uint64_t,h,pv);
	if(hash_map_is_vaild_iter(it))
		pnode = (struct _8puzzle_node*)HASH_MAP_ITER_GET(void*,it);
	else
	{
		pnode = calloc(1,sizeof(*pnode));
		memcpy(pnode->puzzle,p,sizeof(int)*9);
		pnode->_map = pmap;
		LINK_LIST_PUSH_BACK(pmap->mnodes,&pnode->_lnode);
		HASH_MAP_INSERT(uint64_t,void*,h,pv,(void*)pnode);
		int i = 0;
		int j = 0;
		for( ; i < 3; ++i)
		{
			for(j = 0; j < 3; ++j)
				if(p[i][j] == 0)
				{
					pnode->zero_x = j;
					pnode->zero_y = i;
					return pnode;
				}
		}

	}
	return pnode;
}

static inline void swap(int *a, int *b)
{
	int temp = *a;
	*a = *b;
	*b = temp;
}

struct map_node** _8_get_neighbors(struct map_node *mnode)
{
	struct map_node **ret = (struct map_node **)calloc(4,sizeof(*ret));
	struct _8puzzle_node *__8puzzle_node = (struct _8puzzle_node*)mnode;
	struct _8puzzle_map *_puzzle_map = __8puzzle_node->_map;
	int p[3][3];
	int32_t i = 0;
	int32_t c = 0;
	for( ; i < 4; ++i)
	{
		int x = __8puzzle_node->zero_x + direction[i][0];
		int y = __8puzzle_node->zero_y + direction[i][1];
		if(x < 0 || x >=3 || y < 0 || y >= 3)
			continue;
		memcpy(p,__8puzzle_node->puzzle,sizeof(int)*9);
		swap(&p[y][x],&p[__8puzzle_node->zero_y][__8puzzle_node->zero_x]);		
		struct _8puzzle_node *tmp = getnode_by_pv(_puzzle_map,p);
		ret[c++] = (struct map_node*)tmp;
	}
	return ret;
}

double _8_cost_2_neighbor(struct path_node *from,struct path_node *to)
{
	return 1.0f;
}

double _8_cost_2_goal(struct path_node *from,struct path_node *to)
{
	struct _8puzzle_node *_from = (struct _8puzzle_node*)from->_map_node;
	struct _8puzzle_node *_to = (struct _8puzzle_node*)to->_map_node;
	int i = 0;
	int j = 0;
	double sum = 0.0f;
	for( ; i < 3; ++i)
	{
		for(j = 0; j < 3; ++j)
		{
			if(_from->puzzle[i][j] != _to->puzzle[i][j])
				++sum;
		}
	}
	return sum;
}

struct _8puzzle_map *create_map()
{
	struct _8puzzle_map *map = (struct _8puzzle_map *)calloc(1,sizeof(*map));
	map->mnodes = create_link_list();
	map->puzzle_2_mnode = hash_map_create(4096,sizeof(uint64_t),sizeof(void*),_hash_func_,_hash_key_eq_);
	return map;
}

void destroy_map(struct _8puzzle_map **map)
{
	list_node *n = NULL;
	while(n = link_list_pop((*map)->mnodes))
	{
		struct _8puzzle_node *tmp = (struct _8puzzle_node*)((char*)n-sizeof(struct map_node));
		free(tmp);
	}
	free(*map);
}

int8_t check(int *a,int *b)
{
	uint32_t _a = 0;
	uint32_t _b = 0;
	int i = 0;
	int j = 0;
	for( ; i < 9; ++i)
	{
		for( j = i+1;j<9;++j)
			if((a[i]!=0 && a[j]) != 0 && (a[i] > a[j]))
				++_a;
	}
	for( i=0; i < 9; ++i)
	{
		for( j = i+1;j<9;++j)
			if((b[i]!=0 && b[j]) != 0 && (b[i] > b[j]))
				++_b;
	}
	return _a%2 == _b%2;
}

int main()
{
	{	
		int f[3][3] = {
			{2,3,4},
			{1,8,5},
			{0,7,6},
		};
		
		int t[3][3] = {
			{1,2,3},
			{8,0,4},
			{7,6,5},
		};		
		if(!check(&f[0][0],&t[0][0]))
		{
			printf("no way\n");
		}
		else
		{
			struct _8puzzle_map *map = create_map();
			struct map_node *from = (struct map_node*)getnode_by_pv(map,f);
			struct map_node *to = (struct map_node*)getnode_by_pv(map,t);
			struct A_star_procedure *astar = create_astar(_8_get_neighbors,_8_cost_2_neighbor,_8_cost_2_goal);
			struct path_node *path = find_path(astar,from,to);
			printf("\n");
			if(!path)
				printf("no way\n");
			while(path)
			{
				struct _8puzzle_node *mnode = (struct _8puzzle_node *)path->_map_node;
				int i = 0;
				int j = 0;
				for( ; i < 3; ++i)
				{
					for(j = 0; j < 3; ++j)
					{
						if(mnode->puzzle[i][j] == 0)
							printf(" ");
						else
							printf("%d",mnode->puzzle[i][j]);
					}
					printf("\n");
				}
				printf("\n");
				path = path->parent;
			}
			printf("state count:%d\n",hash_map_size(map->puzzle_2_mnode));
			destroy_map(&map);
			destroy_Astar(&astar);	
		}		
	}
	
	{	
		int f[3][3] = {
			{1,2,3},
			{4,5,6},
			{7,8,0},
		};
		
		int t[3][3] = {
			{8,7,6},
			{5,4,3},
			{2,1,0},
		};	
		if(!check(&f[0][0],&t[0][0]))
		{
			printf("no way\n");
		}
		else
		{
		
			struct _8puzzle_map *map = create_map();
			struct map_node *from = (struct map_node*)getnode_by_pv(map,f);
			struct map_node *to = (struct map_node*)getnode_by_pv(map,t);
			struct A_star_procedure *astar = create_astar(_8_get_neighbors,_8_cost_2_neighbor,_8_cost_2_goal);
			struct path_node *path = find_path(astar,from,to);
			printf("\n");
			if(!path)
				printf("no way\n");
			while(path)
			{
				struct _8puzzle_node *mnode = (struct _8puzzle_node *)path->_map_node;
				int i = 0;
				int j = 0;
				for( ; i < 3; ++i)
				{
					for(j = 0; j < 3; ++j)
					{
						if(mnode->puzzle[i][j] == 0)
							printf(" ");
						else
							printf("%d",mnode->puzzle[i][j]);
					}
					printf("\n");
				}
				printf("\n");
				path = path->parent;
			}
			printf("state count:%d\n",hash_map_size(map->puzzle_2_mnode));
			destroy_map(&map);
			destroy_Astar(&astar);
		}
	}	
	return 0;
}



