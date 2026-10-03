#ifndef N3D_RE_GUARD_MOVE_H
#define N3D_RE_GUARD_MOVE_H

#include "n3d_re_guard.h"
#include "n3d_re_runtime.h"

#include <stdint.h>

typedef int (*n3d_guard_world_block_callback)(
    int16_t candidate_world_x,
    int16_t candidate_world_y,
    void* user);

typedef struct n3d_guard_movement_result
{
    uint8_t x_blocked;
    uint8_t y_blocked;
    uint8_t position_committed;
    uint8_t bounced;
    int16_t applied_x;
    int16_t applied_y;
} n3d_guard_movement_result;

int N3D_RE_TickGuardVerticalBob(
    n3d_guard_record* guard,
    n3d_object_record* object);

int N3D_RE_GuardMovementCandidateTouchesPlayer(
    int16_t candidate_x,
    int16_t candidate_y,
    int16_t player_world_x,
    int16_t player_world_y);

int16_t N3D_RE_GuardDirectionPadding(int8_t component);

void N3D_RE_AdvanceGuardMovementFrame(
    n3d_guard_record* guard,
    n3d_object_record* object);

n3d_guard_movement_result
N3D_RE_TickGuardMovementCollisionCoreWithRng(
    n3d_guard_record* guard,
    n3d_object_record* object,
    n3d_guard_world_block_callback is_blocked_at,
    void* user,
    uint32_t* rng_state);

n3d_guard_movement_result
N3D_RE_TickGuardMovementCollisionCore(
    n3d_guard_record* guard,
    n3d_object_record* object,
    n3d_guard_world_block_callback is_blocked_at,
    void* user);

#endif
