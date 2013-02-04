/*	
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/	
#ifndef _RBTREE_H
#define _RBTREE_H
#include <stdint.h>
typedef struct rbtree *rbtree_t;

typedef struct rbnode{
	struct rbnode *parent;
	struct rbnode *left;
	struct rbnode *right;
	void   *key;
	void   *val;
	rbtree_t tree;
	uint8_t color;	
}rbnode;

typedef int32_t (*cmp_function)(void*,void*);

typedef struct rbtree{
	rbnode *root;
	rbnode *nil;
	uint32_t size;
	uint16_t key_size;
	uint16_t val_size;	
	cmp_function compare_function;
	rbnode  dummy;
}rbtree;

rbtree_t create_rbtree(int32_t k_size,int32_t v_size,cmp_function);
void     destroy_rbtree(rbtree_t *);
int8_t   rbtree_insert(rbtree_t,rbnode*);
rbnode*  rbtree_find(rbtree_t,void *key);
rbnode*  rbtree_erase(rbtree_t,rbnode*);
rbnode*  rbtree_remove(rbtree_t,void *key);
void     rbtree_check_vaild(rbtree_t rb);
rbnode*  rbtree_first(rbtree_t);
rbnode*  rbtree_last(rbtree_t);
rbnode*  rbnode_next(rbtree_t,rbnode*);
rbnode*  rbnode_pre(rbtree_t,rbnode*);
#endif

