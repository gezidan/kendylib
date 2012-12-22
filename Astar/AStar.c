#include "AStar.h"
#include "common_hash_function.h"
#include <float.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#ifdef _MINHEAP_

//use minheap as openlist
minheap_t minheap_create(int32_t size,int8_t (*less)(struct path_node*l,struct path_node*r))
{
	minheap_t m = calloc(1,sizeof(*m) + (size * sizeof(struct path_node*)));
	m->size = 0;
	m->max_size = size;
	m->less = less;
	return m;
}

static inline void swap(minheap_t m,int32_t idx1,int32_t idx2)
{
	struct path_node *ele = m->data[idx1];
	m->data[idx1] = m->data[idx2];
	m->data[idx2] = ele;
	m->data[idx1]->heap_index = idx1;
	m->data[idx2]->heap_index = idx2;
}

static inline int32_t parent(int32_t idx)
{
	return idx/2;
}

static inline int32_t left(int32_t idx)
{
	return idx*2;
}

static inline int32_t right(int32_t idx)
{
	return idx*2+1;
}

static inline void up(minheap_t m,int32_t idx)
{
	int32_t p = parent(idx);
	while(p)
	{
		assert(m->data[idx]);
		assert(m->data[p]);
		if(m->less(m->data[idx],m->data[p]))
		{
			swap(m,idx,p);
			idx = p;
			p = parent(idx);
		}
		else
			break;
	}
}

static inline void down(minheap_t m,int32_t idx)
{
	int32_t l = left(idx);
	int32_t r = right(idx);
	int32_t min = idx;
	if(l < m->size)
	{
		assert(m->data[l]);
		assert(m->data[idx]);
	}
	
	if(l < m->size && m->less(m->data[l],m->data[idx]))
		min = l;
	
	if(r < m->size)
	{
		assert(m->data[r]);
		assert(m->data[min]);
	}
	
	if(r < m->size && m->less(m->data[r],m->data[min]))
			min = r;
	
	if(min != idx)
	{
		swap(m,idx,min);
		down(m,min);
	}		
}

void minheap_destroy(minheap_t *m)
{
	free(*m);
	*m = NULL;
}


void minheap_change(minheap_t m,struct path_node *e)
{
	int idx = e->heap_index;
	down(m,idx);
	up(m,idx);
}

void minheap_insert(minheap_t m,struct path_node *e)
{
	if(e->heap_index)
		return minheap_change(m,e);	
	if(m->size >= m->max_size)
		return;
	++m->size;
	m->data[m->size] = e;
	e->heap_index = m->size;
	up(m,e->heap_index);	
}

void minheap_remove(minheap_t m,struct path_node *e)
{
	struct path_node **back = calloc(1,sizeof(struct path_node*)*(m->size-1));
	int32_t i = 1;
	int32_t c = 1;
	for( ; i <= m->size;++i)
	{
		m->data[i]->heap_index = 0;
		if(m->data[i] != e)
			back[c++] = m->data[i];
	}
	memset(&(m->data[0]),0,sizeof(struct path_node*)*m->max_size);
	m->size = 0;
	i = 1;
	for(; i < c; ++i)
		minheap_insert(m,back[i]);
	free(back);
}





//return the min element
static inline struct path_node* minheap_min(minheap_t m)
{
	if(!m->size)
		return NULL;
	return m->data[1];
}

//return the min element and remove it
static inline struct path_node* minheap_popmin(minheap_t m)
{
	if(m->size)
	{
		struct path_node *e = m->data[1];
		swap(m,1,m->size);
		m->data[m->size] = NULL;
		--m->size;
		down(m,1);
		e->heap_index = 0;
		return e;
	}
	return NULL;
}


//返回path_node是否在open表中
static inline int8_t is_in_openlist(struct path_node *pnode)
{
	return pnode->heap_index > 0;
	//return pnode->_open_list_node.pre && pnode->_open_list_node.next;
}

//返回path_node是否在close表中
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
		LINK_LIST_PUSH_BACK(astar->pnodes,pnode);
		HASH_MAP_INSERT(void*,void*,astar->mnode_2_pnode,(void*)mnode,(void*)pnode);
	}
	return pnode;
}

static inline struct path_node *remove_min_pnode(struct A_star_procedure *astar)
{
	return minheap_popmin(astar->open_list);
}

static inline void insert_2_open(struct A_star_procedure *astar,struct path_node *pnode)
{
	minheap_insert(astar->open_list,pnode);
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
		tmp->_close_list_node.pre = tmp->_close_list_node.next = NULL;
		tmp->parent = NULL;
		tmp->G = tmp->H = tmp->F = 0.0f;
		n = n->next;
	}
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
		if(current_node == _to)
			return current_node;
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
					double new_G = current_node->G + astar->_cost_2_neighbor(current_node,neighbor);
					if(new_G < neighbor->G)
					{
						//经过当前neighbor路径更佳,更新路径
						neighbor->G = new_G;
						neighbor->F = neighbor->G + neighbor->H;
						neighbor->parent = current_node;
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

static inline int8_t _less(struct path_node*l,struct path_node*r)
{
	return l->F < r->F;
}

struct A_star_procedure *create_astar(get_neighbors _get_neighbors,cost_2_neighbor _cost_2_neighbor,cost_2_goal _cost_2_goal)
{
	struct A_star_procedure *astar = (struct A_star_procedure *)calloc(1,sizeof(*astar));
	astar->_get_neighbors = _get_neighbors;
	astar->_cost_2_neighbor = _cost_2_neighbor;
	astar->_cost_2_goal = _cost_2_goal;
	//double_link_clear(&astar->open_list);
	astar->open_list = minheap_create(40960,_less);
	double_link_clear(&astar->close_list);
	astar->pnodes = create_link_list();
	astar->mnode_2_pnode = hash_map_create(81920,sizeof(void*),sizeof(void*),_hash_func_,_hash_key_eq_);
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
	minheap_destroy(&astar->open_list);
	destroy_link_list(&astar->pnodes);
	hash_map_destroy(&astar->mnode_2_pnode);
	free(astar);
	*_astar = NULL;
}

#else

//返回path_node是否在open表中
static inline int8_t is_in_openlist(struct path_node *pnode)
{
	return pnode->_open_list_node.pre && pnode->_open_list_node.next;
}

//返回path_node是否在close表中
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
	struct double_link_node *dn = 	astar->open_list.head.next;
	while(dn != &astar->open_list.tail)
	{
		struct path_node *tmp = (struct path_node*)((char*)dn-sizeof(struct list_node));
		if(tmp->F < min_F)
		{
			min_F = tmp->F;
			min_node = tmp;
		}
		dn = dn->next;
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
		if(current_node == _to)
			return current_node;
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
					double new_G = current_node->G + astar->_cost_2_neighbor(current_node,neighbor);
					if(new_G < neighbor->G)
					{
						//经过当前neighbor路径更佳,更新路径
						neighbor->G = new_G;
						neighbor->F = neighbor->G + neighbor->H;
						neighbor->parent = current_node;
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


struct A_star_procedure *create_astar(get_neighbors _get_neighbors,cost_2_neighbor _cost_2_neighbor,cost_2_goal _cost_2_goal)
{
	struct A_star_procedure *astar = (struct A_star_procedure *)calloc(1,sizeof(*astar));
	astar->_get_neighbors = _get_neighbors;
	astar->_cost_2_neighbor = _cost_2_neighbor;
	astar->_cost_2_goal = _cost_2_goal;
	double_link_clear(&astar->open_list);
	double_link_clear(&astar->close_list);
	astar->pnodes = create_link_list();
	astar->mnode_2_pnode = hash_map_create(40960,sizeof(void*),sizeof(void*),_hash_func_,_hash_key_eq_);
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
#endif
