#ifndef N3D_RE_PROJECTILE_H
#define N3D_RE_PROJECTILE_H

#include "n3d_re_player.h"
#include "n3d_re_runtime.h"

#include <stdint.h>

#define N3D_PROJECTILE_GUARD_HIT_TOLERANCE 9
#define N3D_PROJECTILE_RENDER_DISTANCE_THRESHOLD 20

typedef struct n3d_projectile_sequence_offsets
{
    uint8_t flight;
    uint8_t impact;
} n3d_projectile_sequence_offsets;

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

int N3D_RE_EnterProjectileImpact(
    int slot,
    uint8_t weapon_selector,
    uint8_t sequence_base);

void N3D_RE_AdvanceProjectileAnimation(int slot, int frame_count);

#endif
