#ifndef N3D_RE_GUARD_H
#define N3D_RE_GUARD_H

#include "n3d_re_runtime.h"

#include <stdint.h>

#define N3D_GUARD_WAKE_CACHE_SIZE 64
#define N3D_GUARD_AREA_UNSET 0xFF
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
extern uint32_t n3d_original_rng_state;

uint16_t N3D_RE_RngNext(uint32_t* state);
void N3D_RE_ResetOriginalRng(void);
uint16_t N3D_RE_RngNextGlobal(void);
n3d_guard_move_vector N3D_RE_GuardDirectionalStep(uint8_t facing, uint8_t strategy);
uint16_t N3D_RE_State13InitialTimer(uint16_t random_value);

int N3D_RE_TickAnimationTimer(
    n3d_guard_record* guard,
    n3d_object_record* object);

int N3D_RE_BeginPackedSequence(
    n3d_guard_record* guard,
    n3d_object_record* object,
    uint16_t sequence_value,
    uint8_t state,
    uint8_t next_state);

int N3D_RE_BeginState02AlertSequence(
    n3d_guard_record* guard,
    n3d_object_record* object,
    uint16_t class_sequence_34);

int N3D_RE_BeginState03AttackSequence(
    n3d_guard_record* guard,
    n3d_object_record* object,
    uint16_t class_sequence_36);

int N3D_RE_BeginState04RecoverySequence(
    n3d_guard_record* guard,
    n3d_object_record* object,
    uint16_t class_sequence_38);

/*
 * Recovered FUN_1010_7594 decision tail.
 * Writes +0x17 perception cache and +0x18 one-tile proximity cache.
 * Returns 1 when +0x16 mode is recovered (0..2), 0 for unsupported modes.
 */
int N3D_RE_TryEvaluateGuardAttackGate(
    n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    int perception_succeeded,
    int* attack_eligible);

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

/* FUN_247A/71DC: update only on class-0x44 AREA cells; otherwise preserve. */
int N3D_RE_UpdateGuardAreaForSlot(uint16_t guard_slot);

/* FUN_7664 one-shot AREA wake scan. AREA 0 is an original no-op. */
int N3D_RE_WakeGuardsWithRng(uint8_t area_id, uint32_t* rng_state);
int N3D_RE_WakeGuards(uint8_t area_id);

#endif
