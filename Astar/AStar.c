#include "AStar.h"
#include "util/common_hash_function.h"
#include <float.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

//����path_node�Ƿ���open����
static inline int8_t is_in_openlist(struct path_node *pnode)
{
	return pnode->_heapele.index > 0;
}

//����path_node�Ƿ���close����
static inline int8_t is_in_closelist(struct path_node *pnode)
{
	return pnode->_close_list_node.pre && pnode->_close_list_node.next;
}

static inline struct path_node *get_pnode_from_mnode(struct A_star_procedure *astar,struct map_node *mnode)
{
	struct path_node *pnode = NULL;
	hash_map_iter it = HASH_MAP_FIND(void*,astar->mnode_2_pnode,(void*)mnode);
	if(hash_map_is_vaild_iter(it))
		pnode = (struct path_node *)HASH_MAP_ITER_GET(void*,it);
	else
	{
		pnode = calloc(1,sizeof(*pnode));
		pnode->_map_node = mnode;
		HASH_MAP_INSERT(void*,void*,astar->mnode_2_pnode,(void*)mnode,(void*)pnode);
	}
	return pnode;
}

static inline struct path_node *remove_min_pnode(struct A_star_procedure *astar)
{
	struct heapele *hele = minheap_popmin(astar->open_list);
	return (struct path_node*)hele;
}

static inline void insert_2_open(struct A_star_procedure *astar,struct path_node *pnode)
{
	minheap_insert(astar->open_list,(struct heapele*)pnode);
}

static inline void insert_2_close(struct A_star_procedure *astar,struct path_node *pnode)
{
	double_link_push(&astar->close_list,&pnode->_close_list_node);
}

static inline void change_open(struct A_star_procedure *astar,struct path_node *pnode)
{
	minheap_change(astar->open_list,(struct heapele*)pnode);
}

static inline init_state(struct A_star_procedure *astar)
{
	struct double_link_node *dln = NULL;
	while(dln = double_link_pop(&astar->close_list));
	minheap_clear(astar->open_list,NULL);
}

struct path_node* find_path(struct A_star_procedure *astar,struct map_node *from,struct map_node *to)
{
	init_state(astar);
	struct path_node *_from = get_pnode_from_mnode(astar,from);
	if(from == to)
		return _from;//��ʼ����Ŀ�����ͬһ���ڵ�
	struct path_node *_to = get_pnode_from_mnode(astar,to);
	insert_2_open(astar,_from);
	struct path_node *current_node = NULL;	
	while(1)
	{	
		current_node = remove_min_pnode(astar);
		if(!current_node)
			return NULL;//��·�ɴ�
		if(current_node == _to)
			return current_node;
		//current���뵽close��
		insert_2_close(astar,current_node);	
		//��ȡcurrent�����ڽڵ�
		struct map_node **neighbors = astar->_get_neighbors(current_node->_map_node);
		if(neighbors)
		{
			uint32_t i = 0;
			for( ; ; i++)
			{
				if(NULL == neighbors[i])
					break;
				struct path_node *neighbor = get_pnode_from_mnode(astar,neighbors[i]);
				if(is_in_closelist(neighbor))
					continue;//��close����,��������
				if(is_in_openlist(neighbor))
				{
					double new_G = current_node->G + astar->_cost_2_neighbor(current_node,neighbor);
					if(new_G < neighbor->G)
					{
						//������ǰneighbor·������,����·��
						neighbor->G = new_G;
						neighbor->F = neighbor->G + neighbor->H;
						neighbor->parent = current_node;
						change_open(astar,neighbor);
					}
					continue;
				}
				neighbor->parent = current_node;
				neighbor->G = current_node->G + astar->_cost_2_neighbor(current_node,neighbor);
				neighbor->H = astar->_cost_2_goal(neighbor,_to);
				neighbor->F = neighbor->G + neighbor->H;
				insert_2_open(astar,neighbor);					
			}
			free(neighbors);
			neighbors = NULL;
		}	
	}
}

static int32_t _hash_key_eq_(void *l,void *r)
{
	struct map_node** _l = (struct map_node**)l;
	struct map_node** _r = (struct map_node**)r;
	if(*_r == *_l)
		return 0;
	return -1;
}

static uint64_t _hash_func_(void* key)
{
	return burtle_hash(key,sizeof(key),1);
}

static inline int8_t _less(struct heapele*l,struct heapele*r)
{
	struct path_node *_l = (struct path_node*)l;
	struct path_node *_r = (struct path_node*)r;  
	return _l->F < _r->F;
}

struct A_star_procedure *create_astar(get_neighbors _get_neighbors,cost_2_neighbor _cost_2_neighbor,cost_2_goal _cost_2_goal)
{
	struct A_star_procedure *astar = (struct A_star_procedure *)calloc(1,sizeof(*astar));
	astar->_get_neighbors = _get_neighbors;
	astar->_cost_2_neighbor = _cost_2_neighbor;
	astar->_cost_2_goal = _cost_2_goal;
	astar->open_list = minheap_create(8192,_less);
	double_link_clear(&astar->close_list);
	astar->mnode_2_pnode = hash_map_create(8192,sizeof(void*),sizeof(void*),_hash_func_,_hash_key_eq_);
	return astar;
}

void   destroy_Astar(struct A_star_procedure **_astar)
{
	struct A_star_procedure *astar = *_astar;
	hash_map_iter it = hash_map_begin(astar->mnode_2_pnode);
	while(hash_map_is_vaild_iter(it))
	{
		struct path_node *tmp = HASH_MAP_ITER_GET(struct path_node*,it);
		free(tmp);
		it = hash_map_iter_next(it);
	}
	minheap_destroy(&astar->open_list);
	hash_map_destroy(&astar->mnode_2_pnode);
	free(astar);
	*_astar = NULL;
}
