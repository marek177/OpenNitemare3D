#include "m_obj.h"

obj_t* m_objects[MAXOBJ];
int m_objcount;

void M_Update()
{
    for(int i = 0; i < m_objcount; i++)
    {
        obj_t* obj = m_objects[i];
        obj->x += obj->velx;
        obj->y += obj->vely;
    }
}

obj_t* M_Spawn(m_type type, int x, int y, angle_t angle)
{
    if(m_objcount >= MAXOBJ)
        return NULL;

    obj_t* object = calloc(1, sizeof(*object));
    if(!object)
        return NULL;

    object->x = x;
    object->y = y;
    object->angle = angle;
    object->type = type;

    m_objects[m_objcount] = object;
    m_objcount++;

    return object;
}
