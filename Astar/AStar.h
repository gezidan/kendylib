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
#include <stdint.h>
#include <stdlib.h>
#include "util/double_link.h"
#include "util/link_list.h"
#include "util/hash_map.h"
#include "util/minheap.h"

//һ����ͼ��ڵ�
struct map_node{};

//·���ڵ�
struct path_node
{	
	struct heapele _heapele;	
	struct double_link_node _close_list_node;
	struct path_node *parent;
	struct map_node  *_map_node;
	double G;//�ӳ�ʼ�㵽��ǰ��Ŀ���
	double H;//�ӵ�ǰ�㵽Ŀ���Ĺ��ƿ���
	double F;
};

//��ʹ�����ṩ��3������
//get_neighborsԼ��:���һ��map_node*���赲��,�����ᱻ����
typedef struct map_node** (*get_neighbors)(struct map_node*);
typedef double (*cost_2_neighbor)(struct path_node*,struct path_node*);
typedef double (*cost_2_goal)(struct path_node*,struct path_node*);

//һ��·�������Ĺ��̶���
struct A_star_procedure
{
	get_neighbors _get_neighbors;
	cost_2_neighbor _cost_2_neighbor;//���ڼ�������·����Gֵ�ĺ���ָ��
	cost_2_goal _cost_2_goal;//���ڼ�������·����Hֵ�ĺ���ָ��
	minheap_t open_list;
	struct double_link close_list;
	hash_map_t mnode_2_pnode;//map_node��path_node��ӳ��
};

struct A_star_procedure *create_astar(get_neighbors,cost_2_neighbor,cost_2_goal);
//Ѱ�Ҵ�from��to��·��,�ҵ�����·����,���򷵻�NULL
struct path_node* find_path(struct A_star_procedure *astar,struct map_node *from,struct map_node *to);
void   destroy_Astar(struct A_star_procedure**);
