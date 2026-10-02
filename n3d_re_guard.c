#include "n3d_re_guard.h"

#include <string.h>

uint8_t n3d_guard_wake_cache[N3D_GUARD_WAKE_CACHE_SIZE];

uint16_t N3D_RE_RngNext(uint32_t* state)
{
    if (!state)
        return 0;

    *state = (*state * 0x343FDu) + 0x269EC3u;
    return (uint16_t)((*state >> 16) & 0x7FFFu);
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

int N3D_RE_DoorSelectorFromWallId(uint8_t wall_id, uint8_t* selector)
{
    if (!selector)
        return 0;

    const int is_door =
        (wall_id >= 0x70 && wall_id <= 0x77) ||
        (wall_id >= 0x79 && wall_id <= 0x80) ||
        (wall_id >= 0x82 && wall_id <= 0x83) ||
        (wall_id >= 0xA3 && wall_id <= 0xA6) ||
        (wall_id >= 0xAD && wall_id <= 0xAE);

    if (!is_door)
        return 0;

    *selector = (uint8_t)(wall_id - 0x70);
    return 1;
}

int N3D_RE_WakeGuards(uint8_t selector, uint32_t* rng_state)
{
    if (selector == 0 || selector >= N3D_GUARD_WAKE_CACHE_SIZE)
        return 0;

    if (n3d_guard_wake_cache[selector])
        return 0;

    n3d_guard_wake_cache[selector] = 1;

    int woke = 0;
    for (uint16_t i = 0; i < n3d_guard_count; ++i)
    {
        n3d_guard_record* guard = &n3d_guards[i];

        if (guard->strategy != 0 ||
            guard->definition_id != selector ||
            (guard->state != N3D_GUARD_STATE_07 &&
             guard->state != N3D_GUARD_STATE_08))
        {
            continue;
        }

        const uint16_t random_value = N3D_RE_RngNext(rng_state);
        guard->timer = (int16_t)(random_value % 8);
        guard->state = N3D_GUARD_STATE_01;
        ++woke;
    }

    return woke;
}
