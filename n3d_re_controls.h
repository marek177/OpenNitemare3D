#ifndef N3D_RE_CONTROLS_H
#define N3D_RE_CONTROLS_H

#include "n3d_re_player.h"
#include "n3d_re_timing.h"

#include <stdint.h>

typedef struct n3d_control_steps
{
    uint16_t movement_substeps;
    uint16_t turn_degrees;
} n3d_control_steps;

n3d_control_steps N3D_RE_ControlStepsForInput(uint16_t input_mask);

int N3D_RE_ForwardAngle(int player_angle);
int N3D_RE_BackwardAngle(int player_angle);
int N3D_RE_StrafeLeftAngle(int player_angle);
int N3D_RE_StrafeRightAngle(int player_angle);
int N3D_RE_LeftTurnDelta(uint16_t input_mask);
int N3D_RE_RightTurnDelta(uint16_t input_mask);

#endif
