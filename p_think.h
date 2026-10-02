#ifndef P_THINK
#define P_THINK

#include "m_obj.h"
#include "i_keyboard.h"
#include "n3d_re_player.h"
#include "n3d_re_use.h"
#include <math.h>

extern obj_t* p_player;
extern int p_use_rising_edge;

void P_InitPlayer(obj_t* player);
void P_PlayerThink();
void P_PlayerHandleInput();

#endif
