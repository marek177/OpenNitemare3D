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
    uint8_t guards_woken;
} n3d_fire_result;

typedef struct n3d_weapon_jam_event_result
{
    uint8_t handled;
    uint8_t jammed;
    uint8_t sound_id;
} n3d_weapon_jam_event_result;

#define N3D_WEAPON_JAM_ENABLE_EVENT 0x47
#define N3D_WEAPON_JAM_DISABLE_EVENT 0x48
#define N3D_WEAPON_JAM_SOUND_ID 0x44

/*
 * Executes only the recovered pre-fire transaction.
 * Exact cadence/cooldown remains outside until its scheduler is closed.
 * For projectile weapons a free slot is reserved/initialized before ammo is
 * decremented. Silver Pistol returns HITSCAN_READY without projectile use.
 */
n3d_fire_result N3D_RE_TryBeginPlayerFire(uint8_t projectile_sequence_base);
n3d_weapon_jam_event_result N3D_RE_ApplyWeaponJamEvent(uint8_t event_id);

#endif
