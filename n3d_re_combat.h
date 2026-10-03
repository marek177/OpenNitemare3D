#ifndef N3D_RE_COMBAT_H
#define N3D_RE_COMBAT_H

#include "n3d_re_player.h"
#include "n3d_re_runtime.h"

#include <stdint.h>

#define N3D_GUARD_FRESH_STRENGTH 0xFF
#define N3D_GUARD_PAIN_RESULT_OCTANT 8
#define N3D_MAX_DAMAGE 255
#define N3D_HAMERSTEIN_GATE_REQUIRED 3
#define N3D_HAMERSTEIN_BASE_DAMAGE 3

typedef enum n3d_guard_hit_result
{
    N3D_GUARD_HIT_NO_DAMAGE,
    N3D_GUARD_HIT_PAIN,
    N3D_GUARD_HIT_SPECIAL_REACTION_REQUIRED,
    N3D_GUARD_HIT_KILLED,
    N3D_GUARD_HIT_DRACULA_TRANSFORMED
} n3d_guard_hit_result;

typedef struct n3d_damage_result
{
    int raw_seed;
    int class_transformed;
    int difficulty_transformed;
    uint8_t stored_byte;
} n3d_damage_result;

typedef struct n3d_guard_attack_damage_result
{
    int distance_seed;
    int class_transformed;
    int difficulty_transformed;
    uint8_t stored_byte;
} n3d_guard_attack_damage_result;

int N3D_RE_ScalePlayerDamageByDifficulty(int damage, uint8_t difficulty);
int N3D_RE_ScaleEnemyDamageByDifficulty(int damage, uint8_t difficulty);

int N3D_RE_GuardAttackDamageUsesRandom(uint8_t object_class);
int N3D_RE_OriginalRoundedSqrt(int squared_distance);
int N3D_RE_ComputeGuardAttackDistanceMetric(
    int16_t guard_world_x,
    int16_t guard_world_y,
    int16_t player_world_x,
    int16_t player_world_y);

int N3D_RE_ApplyGuardAttackClassTransform(
    int distance_seed,
    uint8_t object_class,
    int class16_full_damage_gate,
    uint16_t rng_value);

n3d_guard_attack_damage_result N3D_RE_ComputeGuardToPlayerDamage(
    int distance_metric,
    uint8_t object_class,
    uint8_t difficulty,
    int class16_full_damage_gate,
    uint16_t rng_value);

n3d_guard_attack_damage_result N3D_RE_ComputeGuardToPlayerDamageFromWorld(
    int16_t guard_world_x,
    int16_t guard_world_y,
    int16_t player_world_x,
    int16_t player_world_y,
    uint8_t object_class,
    uint8_t difficulty,
    int class16_full_damage_gate,
    uint16_t rng_value);

int N3D_RE_ApplyClassWeaponDamageTransform(
    int raw_damage,
    uint8_t object_class,
    uint8_t weapon_selector,
    uint8_t hamerstein_gate_value);

n3d_damage_result N3D_RE_ComputePlayerGuardDamage(
    int16_t projected_base_row,
    int16_t viewport_center_y,
    uint8_t object_class,
    uint8_t weapon_selector,
    uint8_t difficulty,
    uint8_t hamerstein_gate_value,
    uint16_t rng_value);

n3d_guard_hit_result N3D_RE_ApplyGuardDamage(uint16_t guard_slot, uint8_t damage);
int N3D_RE_GuardScoreForClass(uint8_t object_class);

#endif
