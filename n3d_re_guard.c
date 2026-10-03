#include "n3d_re_guard.h"

#include "n3d_re_collision.h"
#include "n3d_re_definitions.h"

#include <string.h>

uint8_t n3d_guard_wake_cache[N3D_GUARD_WAKE_CACHE_SIZE];
uint32_t n3d_original_rng_state = 1u;

uint16_t N3D_RE_RngNext(uint32_t* state)
{
    if (!state)
        return 0;

    *state = (*state * 0x343FDu) + 0x269EC3u;
    return (uint16_t)((*state >> 16) & 0x7FFFu);
}

void N3D_RE_ResetOriginalRng(void)
{
    n3d_original_rng_state = 1u;
}

uint16_t N3D_RE_RngNextGlobal(void)
{
    return N3D_RE_RngNext(&n3d_original_rng_state);
}

n3d_guard_move_vector N3D_RE_GuardDirectionalStep(uint8_t facing, uint8_t strategy)
{
    static const int8_t dx[8] = {0, 1, 1, 0, 0, -1, -1, 0};
    static const int8_t dy[8] = {-1, 0, 0, 1, 1, 0, 0, -1};

    const uint8_t index = (uint8_t)(facing & 7u);
    const int8_t scale = strategy == 2 ? 16 : 8;

    n3d_guard_move_vector result = {
        (int8_t)(dx[index] * scale),
        (int8_t)(dy[index] * scale)
    };
    return result;
}

uint16_t N3D_RE_State13InitialTimer(uint16_t random_value)
{
    return (uint16_t)(random_value % N3D_STATE13_RANDOM_RANGE + N3D_STATE13_TIMER_MIN);
}

int N3D_RE_TickAnimationTimer(
    n3d_guard_record* guard,
    n3d_object_record* object)
{
    if(!guard ||
       !object ||
       guard->state != N3D_GUARD_STATE_00)
        return 0;

    const int first_frame =
        guard->definition_value & 0xFF;
    const int frame_count =
        (guard->definition_value >> 8) & 0xFF;

    int frame =
        ((uint8_t)object->animation_frame + 1) & 0xFF;

    if(first_frame + frame_count <= frame)
        frame = first_frame;

    object->animation_frame =
        (int8_t)(uint8_t)frame;

    --guard->timer;

    if(guard->timer <= 0)
    {
        guard->state = guard->next_state;
        return 2; /* transitioned */
    }

    return 1; /* waiting */
}

int N3D_RE_BeginPackedSequence(
    n3d_guard_record* guard,
    n3d_object_record* object,
    uint16_t sequence_value,
    uint8_t state,
    uint8_t next_state)
{
    if(!guard || !object)
        return 0;

    guard->definition_value = sequence_value;
    object->animation_frame =
        (int8_t)(uint8_t)sequence_value;
    guard->timer =
        (int16_t)(((sequence_value >> 8) & 0xFFu) - 1);
    guard->next_state = next_state;
    guard->state = state;
    return 1;
}

int N3D_RE_BeginState02AlertSequence(
    n3d_guard_record* guard,
    n3d_object_record* object,
    uint16_t class_sequence_34)
{
    if(!guard ||
       guard->state != N3D_GUARD_STATE_02)
        return 0;

    return N3D_RE_BeginPackedSequence(
        guard,
        object,
        class_sequence_34,
        N3D_GUARD_STATE_00,
        N3D_GUARD_STATE_03);
}

int N3D_RE_BeginState03AttackSequence(
    n3d_guard_record* guard,
    n3d_object_record* object,
    uint16_t class_sequence_36)
{
    if(!guard ||
       guard->state != N3D_GUARD_STATE_03)
        return 0;

    return N3D_RE_BeginPackedSequence(
        guard,
        object,
        class_sequence_36,
        N3D_GUARD_STATE_00,
        N3D_GUARD_STATE_04);
}

int N3D_RE_BeginState04RecoverySequence(
    n3d_guard_record* guard,
    n3d_object_record* object,
    uint16_t class_sequence_38)
{
    if(!guard ||
       guard->state != N3D_GUARD_STATE_04)
        return 0;

    return N3D_RE_BeginPackedSequence(
        guard,
        object,
        class_sequence_38,
        N3D_GUARD_STATE_00,
        N3D_GUARD_STATE_05);
}

int N3D_RE_TickState01(n3d_guard_record* guard)
{
    if (!guard || guard->state != N3D_GUARD_STATE_01)
        return 0;

    if (guard->timer > 0)
        --guard->timer;

    if (guard->timer <= 0)
    {
        guard->timer = 0;
        guard->state = N3D_GUARD_STATE_02;
    }

    return 1;
}

int N3D_RE_CompleteDeferredState(n3d_guard_record* guard)
{
    if (!guard ||
        (guard->state != N3D_GUARD_STATE_00 &&
         guard->state != N3D_GUARD_STATE_12))
        return 0;

    guard->state = guard->next_state;
    return 1;
}

int N3D_RE_CompleteState06(n3d_guard_record* guard)
{
    if (!guard || guard->state != N3D_GUARD_STATE_06)
        return 0;

    guard->state = N3D_GUARD_STATE_03;
    return 1;
}

int N3D_RE_CompleteState11(n3d_guard_record* guard)
{
    if (!guard || guard->state != N3D_GUARD_STATE_11)
        return 0;

    guard->strategy = 0;
    guard->state = N3D_GUARD_STATE_07;
    return 1;
}

int N3D_RE_CompletePainState15(n3d_guard_record* guard)
{
    if (!guard || guard->state != N3D_GUARD_STATE_PAIN)
        return 0;

    guard->state = guard->next_state;
    return 1;
}

int N3D_RE_ResolveState07Perception(
    n3d_guard_record* guard,
    int processing_gate_set,
    int perception_succeeded,
    uint16_t random_value)
{
    if (!guard || guard->state != N3D_GUARD_STATE_07)
        return 0;

    if (processing_gate_set || !perception_succeeded)
        return 1;

    if (guard->strategy == 3)
    {
        n3d_guard_move_vector move =
            N3D_RE_GuardDirectionalStep(guard->octant, 0);

        guard->timer = (int16_t)N3D_RE_State13InitialTimer(random_value);
        guard->state = N3D_GUARD_STATE_TIMED_DIRECTIONAL_MOVE;
        guard->move_x = move.dx;
        guard->move_y = move.dy;
        return 1;
    }

    guard->state = N3D_GUARD_STATE_02;
    return 1;
}

n3d_state13_step_result N3D_RE_StepState13(uint16_t current_timer, int target_cell_allows_move)
{
    n3d_state13_step_result result = {0, 0, 0, 0, 0};

    if (current_timer == 0)
    {
        result.clear_strategy_and_enter_state2 = 1;
        return result;
    }

    result.next_timer = (uint16_t)(current_timer - 1);

    if (result.next_timer == 8)
    {
        result.play_movement_sound = 1;
        return result;
    }

    if (result.next_timer < 8)
    {
        result.attempt_movement = 1;
        result.commit_movement = target_cell_allows_move ? 1 : 0;
    }

    return result;
}

void N3D_RE_ClearGuardWakeCache(void)
{
    memset(n3d_guard_wake_cache, 0, sizeof(n3d_guard_wake_cache));
}

int N3D_RE_UpdateGuardAreaForSlot(uint16_t guard_slot)
{
    if(guard_slot >= n3d_guard_count)
        return 0;

    n3d_guard_record* guard = &n3d_guards[guard_slot];
    if(guard->object_slot >= n3d_object_count)
        return 0;

    const n3d_object_record* object =
        &n3d_objects[guard->object_slot];

    const int tile_x =
        (int)N3D_RE_WorldToTile(object->world_x);
    const int tile_y =
        (int)N3D_RE_WorldToTile(object->world_y);

    if(tile_x < 0 || tile_y < 0 ||
       tile_x >= N3D_MAP_WIDTH || tile_y >= N3D_MAP_HEIGHT)
        return 0;

    const n3d_map_cell* cell =
        N3D_RE_MapCell((uint8_t)tile_x, (uint8_t)tile_y);
    if(!cell)
        return 0;

    uint8_t area_id = 0;
    if(!N3D_RE_AreaIdFromWallId(cell->wall, &area_id))
    {
        /* Original preserves the previous AREA id off class-0x44 markers. */
        return 0;
    }

    guard->area_id = area_id;
    return 1;
}

int N3D_RE_WakeGuardsWithRng(uint8_t area_id, uint32_t* rng_state)
{
    if(area_id == 0 || area_id >= N3D_GUARD_WAKE_CACHE_SIZE)
        return 0;

    if(n3d_guard_wake_cache[area_id])
        return 0;

    /* FUN_7664 marks the AREA before scanning the GUARD pool. */
    n3d_guard_wake_cache[area_id] = 1;

    int woke = 0;
    for(uint16_t i = 0; i < n3d_guard_count; ++i)
    {
        n3d_guard_record* guard = &n3d_guards[i];

        if(guard->strategy != 0 ||
           guard->area_id != area_id ||
           (guard->state != N3D_GUARD_STATE_07 &&
            guard->state != N3D_GUARD_STATE_08))
        {
            continue;
        }

        const uint16_t random_value =
            N3D_RE_RngNext(rng_state);
        guard->timer = (int16_t)(random_value % 8);
        guard->state = N3D_GUARD_STATE_01;
        ++woke;
    }

    return woke;
}

int N3D_RE_WakeGuards(uint8_t area_id)
{
    return N3D_RE_WakeGuardsWithRng(
        area_id,
        &n3d_original_rng_state);
}
