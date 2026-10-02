#ifndef N3D_RE_GUARD_H
#define N3D_RE_GUARD_H

#include "n3d_re_runtime.h"

#include <stdint.h>

#define N3D_GUARD_WAKE_CACHE_SIZE 64
#define N3D_STATE13_RANDOM_RANGE 0x50
#define N3D_STATE13_TIMER_MIN 8

typedef struct n3d_guard_move_vector
{
    int8_t dx;
    int8_t dy;
} n3d_guard_move_vector;

typedef struct n3d_state13_step_result
{
    uint16_t next_timer;
    uint8_t play_movement_sound;
    uint8_t attempt_movement;
    uint8_t commit_movement;
    uint8_t clear_strategy_and_enter_state2;
} n3d_state13_step_result;

extern uint8_t n3d_guard_wake_cache[N3D_GUARD_WAKE_CACHE_SIZE];

uint16_t N3D_RE_RngNext(uint32_t* state);
n3d_guard_move_vector N3D_RE_GuardDirectionalStep(uint8_t facing, uint8_t strategy);
uint16_t N3D_RE_State13InitialTimer(uint16_t random_value);
int N3D_RE_TickState01(n3d_guard_record* guard);
int N3D_RE_CompleteDeferredState(n3d_guard_record* guard);
int N3D_RE_CompleteState06(n3d_guard_record* guard);
int N3D_RE_CompleteState11(n3d_guard_record* guard);
int N3D_RE_CompletePainState15(n3d_guard_record* guard);
int N3D_RE_ResolveState07Perception(
    n3d_guard_record* guard,
    int processing_gate_set,
    int perception_succeeded,
    uint16_t random_value);
n3d_state13_step_result N3D_RE_StepState13(uint16_t current_timer, int target_cell_allows_move);
void N3D_RE_ClearGuardWakeCache(void);
int N3D_RE_DoorSelectorFromWallId(uint8_t wall_id, uint8_t* selector);
int N3D_RE_WakeGuards(uint8_t selector, uint32_t* rng_state);

#endif
