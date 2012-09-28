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
#ifndef _AOI_H
#define _AOI_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_BITS 1024

struct point2D
{
	int32_t x;
	int32_t y;
};

static inline uint64_t cal_distance_2D(struct point2D *pos1,struct point2D *pos2)
{
	uint64_t tmp1 = abs(pos1->x - pos2->x);
	tmp1 *= tmp1;
	uint64_t tmp2 = abs(pos1->y - pos2->y);
	tmp2 *= tmp2;
	return (uint64_t)sqrt(tmp1 + tmp2);
}

struct bit_set
{
	uint32_t bits[MAX_BITS];
};

static inline void set_bit(struct bit_set *bs,uint32_t index)
{
	uint32_t b_index = index / MAX_BITS;
	index %= sizeof(uint32_t);
	bs->bits[b_index] = bs->bits[b_index] | (1 << index);
}

static inline void clear_bit(struct bit_set *bs,uint32_t index)
{
	uint32_t b_index = index / MAX_BITS;
	index %= sizeof(uint32_t);
	bs->bits[b_index] = bs->bits[b_index] & (~(1 << index));
}

static inline uint8_t is_set(struct bit_set *bs,uint32_t index)
{
	uint32_t b_index = index / MAX_BITS;
	index %= sizeof(uint32_t);
	return bs->bits[b_index] & (1 << index)?1:0;
}

struct double_link_node
{
	struct double_link_node *pre;
	struct double_link_node *next;
};

struct double_link
{
	struct double_link_node head;
	struct double_link_node tail;
};

static inline int32_t double_link_push(struct double_link *dl,struct double_link_node *dln)
{
	if(dln->pre || dln->next)
		return -1;
	dl->tail.pre->next = dln;
	dln->pre = dl->tail.pre;
	dl->tail.pre = dln;
	dln->next = &dl->tail;
	return 0;
}

static inline int32_t double_link_remove(struct double_link_node *dln)
{
	if(!dln->pre || !dln->next)
		return -1;
	dln->pre->next = dln->next;
	dln->next->pre = dln->pre;	
	dln->pre = dln->next = NULL;		
	return 0;
}

static inline void double_link_clear(struct double_link *dl)
{
	dl->head.next = &dl->tail;
	dl->tail.pre = &dl->head;
}

static inline int32_t double_link_empty(struct double_link *dl)
{
	return dl->head.next == &dl->tail ? 1:0;
}

struct map_block
{
	struct double_link aoi_objs;
	uint32_t x;
	uint32_t y;
};

struct aoi_object
{
	struct double_link_node block_node;             //同一个map_block内的对象形成一个列表      
	struct map_block *current_block;	
	uint32_t aoi_object_id; 
	struct bit_set self_view_objs;                  //自己观察到的对象集合
	struct point2D current_pos;                     //当前坐标
	uint32_t view_radius;                           //可视半径
};

#define BLOCK_LENGTH 500 //一个单元格的大小为5米正方形

typedef void (*callback_)(struct aoi_object *me,struct aoi_object *who);

struct map
{
	struct point2D top_left;          //左上角
	struct point2D bottom_right;      //右下角
	uint32_t x_count;
	uint32_t y_count;
	callback_ enter_callback;
	callback_ leave_callback;
	struct map_block blocks[];
};

#define STAND_RADIUS 500//标准视距,视距大于STAND_RADIUS的对象拥有超视距需要特殊处理

struct map *create_map(struct point2D *top_left,struct point2D *bottom_right,callback_ enter_callback,callback_ leave_callback);
void move_to(struct map *m,struct aoi_object *o,struct point2D *new_pos);
int32_t enter_map(struct map *m,struct aoi_object *o);
int32_t leave_map(struct map *m,struct aoi_object *o);

#endif
