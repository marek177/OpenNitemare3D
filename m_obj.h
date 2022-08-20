#ifndef M_OBJ
#define M_OBJ
#include "m_type.h"
#include "typedefs.h"
#include "p_player.h"
#define MAXOBJ 256

typedef enum objstate
{
    OBJSTATE_PROP,
    OBJSTATE_GUARD_ALERT,
    OBJSTATE_GUARD_MOVING,
    OBJSTATE_GUARD_IDLE,
    OBJSTATE_GUARD_MELEE_SWINGING,
    OBJSTATE_GUARD_DEAD
}objstate;

typedef struct obj_t
{
    float x,y, velx, vely;
    int id;
    angle_t angle;
    int health;
    sprite_t sprite;
    m_type type;
    objstate state;
    objstate nextState;
    player_t* player;
}obj_t;

extern obj_t* m_objects[MAXOBJ];
extern int m_objcount;


void M_Update();
obj_t* M_Spawn(m_type type, int x, int y, angle_t angle);

#endif