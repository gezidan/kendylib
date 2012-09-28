#include <stdio.h>
#include "aoi.h"


struct entity
{
	struct aoi_object _aoi_obj;
};

uint32_t next_aoi_id = 0;

struct entity *create_entity()
{
	struct entity *e = calloc(1,sizeof(*e));
	e->_aoi_obj.aoi_object_id = ++next_aoi_id;
	e->_aoi_obj.view_radius = 500;
	return e;
}

#define MAP_LENGTH 5000   //地图是50m*50m的正方形


void on_enter(struct aoi_object *me,struct aoi_object *who)
{
	printf("%u enter %u\n",who->aoi_object_id,me->aoi_object_id);
}

void on_leave(struct aoi_object *me,struct aoi_object *who)
{
	printf("%u leave %u\n",who->aoi_object_id,me->aoi_object_id);
}

struct map *create_scene()
{
	struct point2D left_top = {0,0};
	struct point2D right_bottom = {MAP_LENGTH,MAP_LENGTH};
	struct map *m = create_map(&left_top,&right_bottom,on_enter,on_leave);
	return m;
}

int main()
{
	struct map *m = create_scene();
	struct entity *e1,*e2;
	e1 = create_entity();
	e1->_aoi_obj.current_pos.x = e1->_aoi_obj.current_pos.y = 100;
	e2 = create_entity();
	e2->_aoi_obj.current_pos.x = e2->_aoi_obj.current_pos.y = 200;
	enter_map(m,(struct aoi_object*)e1);
	enter_map(m,(struct aoi_object*)e2);
	printf("leave\n");
	//leave_map(m,(struct aoi_object*)e2);
	struct point2D new_pos = {800,800};
	printf("move\n");
	move_to(m,(struct aoi_object*)e2,&new_pos);
	
	return 0;
}