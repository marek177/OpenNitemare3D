#ifndef N3D_RE_GUARD_PLAN_H
#define N3D_RE_GUARD_PLAN_H

#include "n3d_re_guard.h"
#include "n3d_re_runtime.h"

#include <stdint.h>

int8_t N3D_RE_GuardSignedStepFromDelta(int delta);

void N3D_RE_UpdateGuardOctantFromMovement(
    n3d_guard_record* guard,
    int move_x,
    int move_y);

/*
 * Recovered FUN_76FC strategy planner. These helpers only choose vector/timer/
 * state. They intentionally stop before forced directional-sequence refresh
 * and the immediate FUN_71DC movement attempt.
 */
int N3D_RE_PlanPlayerPursuitMovementWithRng(
    n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    uint8_t difficulty,
    uint32_t* rng_state);

int N3D_RE_PlanStrategy0MovementWithRng(
    n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    uint8_t difficulty,
    uint32_t* rng_state);

int N3D_RE_PlanStrategy1MovementWithRng(
    n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    uint8_t difficulty,
    uint32_t* rng_state,
    int retreat_door_found,
    int16_t retreat_target_world_x,
    int16_t retreat_target_world_y);

int N3D_RE_PlanStrategy2MovementWithRng(
    n3d_guard_record* guard,
    uint32_t* rng_state);

int N3D_RE_PlanStrategyCurrentVectorMovement(
    n3d_guard_record* guard);

int N3D_RE_PlanMovement76FCWithRng(
    n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    uint8_t difficulty,
    uint32_t* rng_state,
    int retreat_door_found,
    int16_t retreat_target_world_x,
    int16_t retreat_target_world_y);

int N3D_RE_PlanMovement76FC(
    n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    uint8_t difficulty,
    int retreat_door_found,
    int16_t retreat_target_world_x,
    int16_t retreat_target_world_y);

#endif
