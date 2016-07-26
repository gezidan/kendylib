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

#ifndef _DOUBLE_LINK_H
#define _DOUBLE_LINK_H


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

static inline int32_t double_link_empty(struct double_link *dl)
{
	return dl->head.next == &dl->tail ? 1:0;
}

static inline struct double_link_node *double_link_first(struct double_link *dl)
{
	if(double_link_empty(dl))
		return NULL;
	return dl->head.next;
}

static inline struct double_link_node *double_link_last(struct double_link *dl)
{
	if(double_link_empty(dl))
		return NULL;
	return dl->tail.pre;
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

static inline struct double_link_node *double_link_pop(struct double_link *dl)
{
	if(double_link_empty(dl))
		return NULL;
	else
	{
		struct double_link_node *n = dl->head.next;
		double_link_remove(n);
		return n;
	}
}

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

static inline void double_link_clear(struct double_link *dl)
{
	dl->head.pre = dl->tail.next = NULL;
	dl->head.next = &dl->tail;
	dl->tail.pre = &dl->head;
}

#endif
