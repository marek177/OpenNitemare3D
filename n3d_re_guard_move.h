#ifndef N3D_RE_GUARD_MOVE_H
#define N3D_RE_GUARD_MOVE_H

#include "n3d_re_guard.h"
#include "n3d_re_object_defs.h"

#include <stdint.h>

typedef int (*n3d_guard_block_callback)(
    int16_t candidate_world_x,
    int16_t candidate_world_y,
    void* user);

typedef enum n3d_guard_dispatch_result
{
    N3D_GUARD_DISPATCH_NOT_HANDLED = 0,
    N3D_GUARD_DISPATCH_WAITING,
    N3D_GUARD_DISPATCH_TRANSITIONED,
    N3D_GUARD_DISPATCH_MOVED,
    N3D_GUARD_DISPATCH_MOVEMENT_BLOCKED
} n3d_guard_dispatch_result;

typedef struct n3d_guard_movement_result
{
    uint8_t x_blocked;
    uint8_t y_blocked;
    uint8_t position_committed;
    uint8_t bounced;
    int16_t applied_x;
    int16_t applied_y;
} n3d_guard_movement_result;

uint8_t N3D_RE_ComputeGuardPlayerOctant(
    uint8_t control,
    int16_t guard_world_x,
    int16_t guard_world_y,
    int16_t player_world_x,
    int16_t player_world_y);

uint8_t N3D_RE_ComputeGuardResultOctant(
    const n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y);

uint16_t N3D_RE_GetGuardDirectionalSequence(
    const n3d_object_definition_header* definition,
    uint8_t state,
    uint8_t strategy,
    uint8_t result_octant);

n3d_guard_dispatch_result N3D_RE_RefreshGuardDirectionalSequence(
    n3d_guard_record* guard,
    n3d_object_record* object,
    const n3d_object_definition_header* definition,
    int16_t player_world_x,
    int16_t player_world_y,
    int force_refresh);

int N3D_RE_TickGuardVerticalBob(
    n3d_guard_record* guard,
    n3d_object_record* object);

int N3D_RE_GuardMovementCandidateTouchesPlayer(
    int16_t candidate_world_x,
    int16_t candidate_world_y,
    int16_t player_world_x,
    int16_t player_world_y);

int16_t N3D_RE_GuardDirectionPadding(int8_t component);

n3d_guard_movement_result
N3D_RE_TickGuardMovementCollisionCoreWithRng(
    n3d_guard_record* guard,
    n3d_object_record* object,
    n3d_guard_block_callback is_blocked_at,
    void* user,
    uint32_t* rng_state);

n3d_guard_movement_result N3D_RE_TickGuardMovementCollisionCore(
    n3d_guard_record* guard,
    n3d_object_record* object,
    n3d_guard_block_callback is_blocked_at,
    void* user,
    uint32_t* rng_state);

n3d_guard_dispatch_result N3D_RE_TickState06Movement(
    n3d_guard_record* guard,
    n3d_object_record* object,
    const n3d_object_definition_header* definition,
    int16_t player_world_x,
    int16_t player_world_y,
    n3d_guard_block_callback is_blocked_at,
    void* user,
    uint32_t* rng_state,
    n3d_guard_movement_result* out_movement);

#endif
