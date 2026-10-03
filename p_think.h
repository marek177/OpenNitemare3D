#ifndef P_THINK
#define P_THINK

#include "m_obj.h"
#include "i_keyboard.h"
#include "n3d_re_player.h"
#include "n3d_re_use.h"
#include "n3d_re_movement.h"
#include "n3d_re_timing.h"
#include "n3d_re_controls.h"
#include "n3d_re_door.h"
#include "n3d_re_pickup.h"

extern obj_t* p_player;
extern int p_use_rising_edge;
extern n3d_use_execution p_last_use_execution;

void P_InitPlayer(obj_t* player);
void P_PlayerThink();
void P_PlayerHandleInput();

#endif
