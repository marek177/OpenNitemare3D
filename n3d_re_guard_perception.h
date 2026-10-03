#ifndef N3D_RE_GUARD_PERCEPTION_H
#define N3D_RE_GUARD_PERCEPTION_H

#include "n3d_re_runtime.h"

#include <stdint.h>

typedef int (*n3d_guard_los_block_callback)(
    int tile_x,
    int tile_y,
    int secondary_cell_checks,
    void* user);

int N3D_RE_GuardPerceptionPrefilter(
    const n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    int ignore_facing);

int N3D_RE_GuardLosCellBlocks(
    uint8_t primary_flags,
    uint8_t secondary_flags,
    int secondary_cell_checks,
    int linked_runtime_record_passable);

int N3D_RE_TraceGuardGridLine(
    int start_x,
    int start_y,
    int delta_x,
    int delta_y,
    int max_steps,
    int secondary_cell_checks,
    n3d_guard_los_block_callback is_intermediate_blocked,
    void* user);

int N3D_RE_GuardMapIntermediateBlocked(
    int tile_x,
    int tile_y,
    int secondary_cell_checks,
    void* user);

int N3D_RE_EvaluateGuardPerceptionMap(
    const n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    int secondary_cell_checks,
    int ignore_facing);

int N3D_RE_TryEvaluateGuardAttackGate(
    n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    int perception_succeeded,
    int* attack_eligible);

#endif
