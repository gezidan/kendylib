#include <stdio.h>
#include "aoi.h"
#include "SysTime.h"

struct entity
{
	struct aoi_object _aoi_obj;
};

uint32_t next_aoi_id = 0;

struct entity *create_entity(uint32_t view_radius)
{
	struct entity *e = calloc(1,sizeof(*e));
	e->_aoi_obj.aoi_object_id = ++next_aoi_id;
	e->_aoi_obj.view_radius = view_radius;
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
	init_system_time(10);
	struct map *m = create_scene();
	struct entity *e1,*e2;
	e1 = create_entity(700);
	e1->_aoi_obj.current_pos.x = e1->_aoi_obj.current_pos.y = 100;
	e2 = create_entity(50);
	e2->_aoi_obj.current_pos.x = e2->_aoi_obj.current_pos.y = 800;
	enter_map(m,(struct aoi_object*)e1);
	enter_map(m,(struct aoi_object*)e2);

	struct point2D new_pos = {550,550};
	printf("move\n");
	move_to(m,(struct aoi_object*)e2,&new_pos);
	new_pos.x = new_pos.y = 700;
	move_to(m,(struct aoi_object*)e2,&new_pos);
	printf("leave map\n");
	leave_map(m,(struct aoi_object*)e2);
	
	return 0;
}
