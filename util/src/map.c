#include "map.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "RBtree.h"

struct map
{
	struct interface_map_container *container;
	int32_t    free_container_on_destroy;
};

map_t    map_create(uint16_t key_size,uint16_t val_size,comp _comp,struct interface_map_container *container)
{
	map_t m = malloc(sizeof(*m));
	if(!m)
		return 0;
	if(container)
	{
		m->container = container;
		m->free_container_on_destroy = 0;
	}	
	else
	{
		m->container = (struct interface_map_container *)RBtree_create(key_size,val_size,_comp);
		if(!m->container)
		{
			free(m);
			return 0;
		}
		m->free_container_on_destroy = 1;
	}
	return m;
}

void     map_destroy(map_t *m)
{
	
}

inline map_iter map_insert(map_t m,void *key,void *val)
{
	assert(m);
	return m->container->insert(m->container,key,val);
}

inline map_iter map_erase(map_t m,map_iter it)
{
	assert(m);
	return m->container->erase(m->container,it);
}

inline void map_remove(map_t m,void *key)
{
	assert(m);
	m->container->remove(m->container,key);
}

inline map_iter map_find(map_t m,void *key)
{
	assert(m);
	return m->container->find(m->container,key);
}

inline map_iter map_begin(map_t m)
{
	assert(m);
	return m->container->begin(m->container);
}

inline map_iter map_end(map_t m)
{
	assert(m);
	return m->container->end(m->container);
}
inline int32_t	map_size(map_t m)
{
	assert(m);
	return m->container->size(m->container);
}
inline int32_t map_empty(map_t m)
{
	assert(m);
	return m->container->empty(m->container);
}