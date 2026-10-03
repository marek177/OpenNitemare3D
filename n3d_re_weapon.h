#ifndef N3D_RE_WEAPON_H
#define N3D_RE_WEAPON_H

#include "n3d_re_player.h"

#include <stdint.h>

typedef enum n3d_fire_result_kind
{
    N3D_FIRE_NONE = 0,
    N3D_FIRE_NO_WEAPON,
    N3D_FIRE_JAMMED,
    N3D_FIRE_NO_AMMO,
    N3D_FIRE_PROJECTILE_POOL_FULL,
    N3D_FIRE_PROJECTILE_READY,
    N3D_FIRE_HITSCAN_READY,
    N3D_FIRE_UNRESOLVED
} n3d_fire_result_kind;

typedef struct n3d_fire_result
{
    n3d_fire_result_kind kind;
    uint8_t weapon_selector;
    uint8_t ammo_before;
    uint8_t ammo_after;
    int8_t projectile_slot;
    uint8_t ammo_consumed;
} n3d_fire_result;

/*
 * Executes only the recovered pre-fire transaction.
 * Exact cadence/cooldown remains outside until its scheduler is closed.
 * For projectile weapons a free slot is reserved/initialized before ammo is
 * decremented. Silver Pistol returns HITSCAN_READY without projectile use.
 */
n3d_fire_result N3D_RE_TryBeginPlayerFire(uint8_t projectile_sequence_base);

#endif
