#ifndef N3D_RE_MOVEMENT_H
#define N3D_RE_MOVEMENT_H

#include "n3d_re_collision.h"
#include "n3d_re_trig.h"

#include <stdint.h>

typedef struct n3d_line_state
{
    int16_t sin_q10;          /* original 4C46 */
    int16_t cos_q10;          /* original 4C48 */
    int16_t axis_flag;        /* original 4C06 */
    int16_t error;            /* original 4C08 */
    int16_t twice_minor;      /* original 4C0A */
    int16_t twice_minor_minus_major; /* original 4C0C */
    int16_t step_x;           /* projectile slot +08 */
    int16_t step_y;           /* projectile slot +0A */
} n3d_line_state;

typedef struct n3d_player_move_result
{
    int requested_x;
    int requested_y;
    int accepted_x;
    int accepted_y;
    uint32_t x_attempts;
    uint32_t y_attempts;
    uint32_t x_blocked;
    uint32_t y_blocked;
    uint8_t unresolved_collision;
    uint8_t entered_tile_event;
} n3d_player_move_result;

/*
 * Collision-test exactly one signed +/-1 world-unit axis substep from the
 * supplied candidate position. Return value:
 *   +1/-1 = accepted signed step
 *   0     = blocked or unresolved (see *resolved)
 */
int N3D_RE_TestPlayerXSubstep(
    int32_t world_x,
    int32_t world_y,
    int signed_step,
    const n3d_collision_callbacks* callbacks,
    int* resolved);

int N3D_RE_TestPlayerYSubstep(
    int32_t world_x,
    int32_t world_y,
    int signed_step,
    const n3d_collision_callbacks* callbacks,
    int* resolved);

/*
 * Reconstructed one-world-unit Bresenham-like movement kernel. The caller
 * supplies the desired signed world delta; frame/input cadence stays outside.
 */
n3d_player_move_result N3D_RE_MovePlayerWorldDelta(
    int delta_x,
    int delta_y,
    const n3d_collision_callbacks* callbacks);

int N3D_RE_InitLineStateFromAngle(
    int angle_degrees,
    n3d_line_state* state);

n3d_player_move_result N3D_RE_MovePlayerAngleSubsteps(
    int angle_degrees,
    uint16_t major_substeps,
    const n3d_collision_callbacks* callbacks);

#endif
