#include "AStar.h"
#include "common_hash_function.h"
#include <float.h>

//返回path_node是否在open表中
static inline int8_t is_in_openlist(struct path_node *pnode)
{
	return pnode->_open_list_node->pre && pnode->_open_list_node->next;
}

//返回path_node是否在close表中
static inline int8_t is_in_closelist(struct path_node *pnode)
{
	return pnode->_open_list_node->pre && pnode->_open_list_node->next;
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
		LINK_LIST_PUSH_BACK(astar->pnodes,pnode);
		HASH_MAP_INSERT(void*,void*,astar->mnode_2_pnode,(void*)mnode,(void*)pnode);
	}
	return pnode;
}

static inline struct path_node *remove_min_pnode(struct A_star_procedure *astar)
{
	if(double_link_empty(&astar->open_list))
		return NULL;
	double min_F = DBL_MAX;
	struct path_node *min_node = NULL;
	double_link_node *dn = 	astar->open_list.head.next;
	while(dn != &astar->open_list.tail)
	{
		struct path_node *tmp = (struct path_node*)(dl+sizeof(struct list_node));
		if(tmp->F < min_F)
		{
			min_F = tmp->F;
			min_node = tmp;
		}
	}
	
	if(min_node)
		double_link_remove(&min_node->_open_list_node);//从openlist中移除
	
	return min_node;
	
}

static inline void insert_2_open(struct A_star_procedure *astar,struct path_node *pnode)
{
	double_link_push(&astar->open_list,&pnode->_open_list_node);
}

static inline void insert_2_close(struct A_star_procedure *astar,struct path_node *pnode)
{
	double_link_push(&astar->close_list,&pnode->_close_list_node);
}

static inline init_state(struct A_star_procedure *astar)
{
	struct list_node *n = link_list_head(astar->pnodes);
	while(n)
	{
		struct path_node *tmp = (struct path_node*)n;
		tmp->_open_list_node.pre = tmp->_open_list_node.next = NULL;
		tmp->_close_list_node.pre = tmp->_close_list_node.next = NULL;
		tmp->parent = NULL;
		tmp->G = tmp->H = tmp->F = 0.0f;
		n = n->next;
	}
	double_link_clear(&astar->open_list);
	double_link_clear(&astar->close_list);
}

struct path_node* find_path(struct A_star_procedure *astar,struct map_node *from,struct map_node *to)
{
	init_state(astar);
	struct path_node *_from = get_pnode_from_mnode(astar,from);
	if(from == to)
		return _from;//起始点与目标点在同一个节点
	struct path_node *_to = get_pnode_from_mnode(astar,to);
	insert_2_open(astar,_from);
	struct path_node *current_node = NULL;	
	while(1)
	{	
		current_node = remove_min_pnode(astar);
		if(!current_node)
			return NULL;//无路可达
		//current插入到close表
		insert_2_close(astar,current_node);	
		//获取current的相邻节点
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
					continue;//在close表中,不做处理
				if(is_in_openlist(neighbor))
				{
					double new_G = current_node->G + astar->_cal_G_value(current_node,neighbor);
					if(new_G < neighbor->G)
					{
						//经过当前neighbor路径更佳,更新路径
						neighbor->G = new_G;
						neignbor->F = neighbor->G + neighbor->H;
						neighbor->parent = current_node;
					}
					continue;
				}
				neighbor->parent = current_node;
				neighbor->G = current_node->G + astar->_cal_G_value(current_node,neighbor);
				neighbor->H = astar->_cal_H_value(neighbor,_to);
				neignbor->F = neighbor->G + neighbor->H;
				insert_2_open(astar,neighbor);					
			}
			free(neighbors);
			neighbors = NULL;
		}	
	}
}

static int32_t _hash_key_eq_(void *l,void *r)
{
	if(r == l)
		return 0;
	return -1;
}

static uint64_t _hash_func_(void* key)
{
	return burtle_hash(key,sizeof(key),1);
}


struct A_star_procedure *create_astar(get_neighbors _get_neighbors,cal_G_value _cal_G_value,cal_H_value _cal_H_value)
{
	struct A_star_procedure *astar = (struct A_star_procedure *)calloc(1,sizeof(*astar));
	astar->_get_neighbors = _get_neighbors;
	astar->_cal_G_value = _cal_G_value;
	astar->_cal_H_value = _cal_H_value;
	double_link_clear(&astar->open_list);
	double_link_clear(&astar->close_list);
	astar->mnode_2_pnode = (4096,sizeof(void*),sizeof(void*),_hash_func_,_hash_key_eq_);
	return astar;
}

void   destroy_Astar(struct A_star_procedure **_astar)
{
	struct A_star_procedure *astar = *_astar;
	struct list_node *n = NULL;
	while(n = link_list_pop(astar->pnodes))
	{
		struct path_node *tmp = (struct path_node*)n;
		free(tmp);
	}
	destroy_link_list(&astar->pnodes);
	hash_map_destroy(&astar->mnode_2_pnode);
	free(astar);
	*_astar = NULL;
}