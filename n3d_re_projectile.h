#ifndef N3D_RE_PROJECTILE_H
#define N3D_RE_PROJECTILE_H

#include "n3d_re_player.h"
#include "n3d_re_runtime.h"
#include "n3d_re_movement.h"

#include <stdint.h>

#define N3D_PROJECTILE_GUARD_HIT_TOLERANCE 9
#define N3D_PROJECTILE_RENDER_DISTANCE_THRESHOLD 20

typedef struct n3d_projectile_sequence_offsets
{
    uint8_t flight;
    uint8_t impact;
} n3d_projectile_sequence_offsets;

typedef enum n3d_projectile_collision_kind
{
    N3D_PROJECTILE_COLLISION_NONE = 0,
    N3D_PROJECTILE_COLLISION_UNRESOLVED,
    N3D_PROJECTILE_COLLISION_GUARD_HIT,
    N3D_PROJECTILE_COLLISION_EXPLODABLE_WALL,
    N3D_PROJECTILE_COLLISION_WALL_DEFERRED
} n3d_projectile_collision_kind;

typedef struct n3d_projectile_collision_result
{
    n3d_projectile_collision_kind kind;
    uint8_t cell_x;
    uint8_t cell_y;
    uint8_t raw_wall_id;
    uint8_t raw_object_id;
    uint8_t mapped_wall_type;
    int16_t object_slot;
    int16_t guard_slot;
    uint8_t event_id;
    uint8_t enter_impact;
} n3d_projectile_collision_result;

typedef struct n3d_projectile_advance_result
{
    uint16_t requested_substeps;
    uint16_t advanced_substeps;
    n3d_projectile_collision_result collision;
} n3d_projectile_advance_result;

#define N3D_EXPLODABLE_WALL_EVENT 0x29
#define N3D_EXPLODABLE_WALL_RUNTIME_CLASS 0x2D

int N3D_RE_WeaponUsesProjectile(uint8_t weapon_selector);
int N3D_RE_ProjectileSequenceOffsets(
    uint8_t weapon_selector,
    n3d_projectile_sequence_offsets* out);
int N3D_RE_FirstFreeProjectileSlot(void);
int N3D_RE_ProjectileHitsGuard(
    int32_t projectile_x,
    int32_t projectile_y,
    int32_t guard_x,
    int32_t guard_y);
int N3D_RE_ProjectileNeedsProjection(
    int32_t projectile_x,
    int32_t projectile_y,
    int32_t player_x,
    int32_t player_y);

int N3D_RE_InitializeProjectile(
    int slot,
    uint8_t weapon_selector,
    int16_t world_x,
    int16_t world_y,
    uint8_t sequence_base);

int N3D_RE_InitializeProjectileFromAngle(
    int slot,
    uint8_t weapon_selector,
    int16_t world_x,
    int16_t world_y,
    uint8_t sequence_base,
    int angle_degrees);

int N3D_RE_AdvanceProjectileLineSubstep(int slot);
int N3D_RE_AdvanceProjectileLineSteps(int slot, uint16_t substeps);

n3d_projectile_collision_result
N3D_RE_ClassifyProjectileCollision(int slot);

n3d_projectile_advance_result
N3D_RE_AdvanceProjectileUntilCollision(
    int slot,
    uint16_t substeps);

int N3D_RE_EnterProjectileImpact(
    int slot,
    uint8_t weapon_selector,
    uint8_t sequence_base);

void N3D_RE_AdvanceProjectileAnimation(int slot, int frame_count);

#endif
