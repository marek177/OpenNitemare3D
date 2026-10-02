#ifndef N3D_RE_PICKUP_H
#define N3D_RE_PICKUP_H

#include "n3d_re_player.h"
#include "n3d_re_runtime.h"

#include <stdint.h>

typedef enum n3d_pickup_result_kind
{
    N3D_PICKUP_NONE = 0,
    N3D_PICKUP_KEY_GRANTED,
    N3D_PICKUP_IDCARD_GRANTED,
    N3D_PICKUP_PENTAGRAM_GRANTED,
    N3D_PICKUP_SCORE_ADDED,
    N3D_PICKUP_HEALTH_ADDED,
    N3D_PICKUP_HEALTH_AT_THRESHOLD,
    N3D_PICKUP_RESTORE_BONUS,
    N3D_PICKUP_WEAPON_GRANTED,
    N3D_PICKUP_RESOURCE_ADDED,
    N3D_PICKUP_RESOURCE_AT_THRESHOLD,
    N3D_PICKUP_AMMO_ADDED,
    N3D_PICKUP_AMMO_AT_THRESHOLD,
    N3D_PICKUP_CRYSTAL_CHARGE_ADDED,
    N3D_PICKUP_CRYSTAL_CHARGE_AT_THRESHOLD,
    N3D_PICKUP_EYE_CHARGE_ADDED,
    N3D_PICKUP_EYE_CHARGE_AT_THRESHOLD,
    N3D_PICKUP_INACTIVE,
    N3D_PICKUP_DEFERRED,
    N3D_PICKUP_UNRESOLVED
} n3d_pickup_result_kind;

typedef enum n3d_ammo_pool
{
    N3D_AMMO_POOL_NONE = 0,
    N3D_AMMO_POOL_SILVER,
    N3D_AMMO_POOL_LASER,
    N3D_AMMO_POOL_WAND
} n3d_ammo_pool;

typedef struct n3d_pickup_result
{
    n3d_pickup_result_kind kind;
    uint8_t object_class;
    uint8_t map_object_id;
    uint8_t variant_index;
    n3d_ammo_pool ammo_pool;
    uint8_t value_before;
    uint8_t value_after;
    int32_t score_delta;
    uint8_t accepted;
} n3d_pickup_result;

typedef struct n3d_pickup_touch_context
{
    n3d_pickup_result last_result;
    uint32_t touch_calls;
    uint32_t accepted_pickups;
} n3d_pickup_touch_context;

int N3D_RE_AddAmmoPickup(uint8_t* ammo);
int N3D_RE_PickupResultAccepted(const n3d_pickup_result* result);
void N3D_RE_DeactivateAcceptedPickup(uint16_t object_slot);
n3d_pickup_result N3D_RE_ApplyPickupObject(uint16_t object_slot);
n3d_pickup_result N3D_RE_ApplyPickupAtCell(uint8_t x, uint8_t y);
void N3D_RE_PlayerPickupTouchCallback(uint8_t x, uint8_t y, void* user);

#endif
