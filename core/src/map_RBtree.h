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
#ifndef _MAP_RBTREE_H
#define _MAP_RBTREE_H
#include <stdint.h>
#include "map.h"

typedef struct RBtree *RBtree_t;
extern RBtree_t RBtree_create(uint16_t,uint16_t,comp _comp);


struct interface_map_container;
extern void     RBtree_destroy(struct interface_map_container**);

///检查是否有违反红黑树性质
extern void      RBtree_check_vaild(RBtree_t);
#endif