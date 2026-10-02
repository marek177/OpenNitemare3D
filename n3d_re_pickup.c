#include "n3d_re_pickup.h"

static n3d_pickup_result N3D_RE_DeferredPickup(
    const n3d_object_record* object)
{
    n3d_pickup_result result = {0};
    if(object)
    {
        result.kind = N3D_PICKUP_DEFERRED;
        result.object_class = object->object_class;
        result.map_object_id = object->map_object_id;
    }
    else
    {
        result.kind = N3D_PICKUP_UNRESOLVED;
    }
    return result;
}

int N3D_RE_AddAmmoPickup(uint8_t* ammo)
{
    if(!ammo || *ammo >= N3D_NORMAL_AMMO_CAP)
        return 0;

    *ammo = (uint8_t)(*ammo + N3D_AMMO_PICKUP_AMOUNT);
    return 1;
}

n3d_pickup_result N3D_RE_ApplyPickupObject(uint16_t object_slot)
{
    if(object_slot >= n3d_object_count)
        return N3D_RE_DeferredPickup(NULL);

    const n3d_object_record* object = &n3d_objects[object_slot];
    n3d_pickup_result result = {0};
    result.object_class = object->object_class;
    result.map_object_id = object->map_object_id;

    switch(object->object_class)
    {
        case 0x2F: /* KEY */
            if(object->map_object_id < 0x05 || object->map_object_id > 0x08)
                return N3D_RE_DeferredPickup(object);

            result.variant_index = (uint8_t)(object->map_object_id - 0x05);
            N3D_RE_GrantInventoryBit(
                &n3d_player.colored_keys,
                result.variant_index);
            result.kind = N3D_PICKUP_KEY_GRANTED;
            return result;

        case 0x30: /* IDCARD */
            if(object->map_object_id < 0x09 || object->map_object_id > 0x0A)
                return N3D_RE_DeferredPickup(object);

            result.variant_index = (uint8_t)(object->map_object_id - 0x09);
            N3D_RE_GrantInventoryBit(
                &n3d_player.id_cards,
                result.variant_index);
            result.kind = N3D_PICKUP_IDCARD_GRANTED;
            return result;

        case 0x3C: /* PENTAGRAM */
            if(object->map_object_id < 0x1F || object->map_object_id > 0x22)
                return N3D_RE_DeferredPickup(object);

            result.variant_index = (uint8_t)(object->map_object_id - 0x1F);
            N3D_RE_GrantInventoryBit(
                &n3d_player.pentagrams,
                result.variant_index);
            result.kind = N3D_PICKUP_PENTAGRAM_GRANTED;
            return result;

        case 0x39: /* AMMO */
        {
            uint8_t* ammo = NULL;

            if(object->map_object_id == 0x29)
            {
                result.ammo_pool = N3D_AMMO_POOL_SILVER;
                ammo = &n3d_player.silver_ammo;
            }
            else if(object->map_object_id == 0x2A)
            {
                result.ammo_pool = N3D_AMMO_POOL_LASER;
                ammo = &n3d_player.laser_ammo;
            }
            else if(object->map_object_id == 0x2B)
            {
                result.ammo_pool = N3D_AMMO_POOL_WAND;
                ammo = &n3d_player.wand_ammo;
            }
            else
            {
                return N3D_RE_DeferredPickup(object);
            }

            result.value_before = *ammo;
            if(N3D_RE_AddAmmoPickup(ammo))
            {
                result.value_after = *ammo;
                result.kind = N3D_PICKUP_AMMO_ADDED;
            }
            else
            {
                result.value_after = *ammo;
                result.kind = N3D_PICKUP_AMMO_AT_THRESHOLD;
            }
            return result;
        }

        /*
         * FOOD/WEAPON/MAGICEYE/CRYSTALB and scripted containers have mapped
         * identities, but their complete item-specific side effects remain
         * intentionally outside this verified subset.
         */
        default:
            return N3D_RE_DeferredPickup(object);
    }
}

n3d_pickup_result N3D_RE_ApplyPickupAtCell(uint8_t x, uint8_t y)
{
    const int object_slot = N3D_RE_FindObjectSlotByCell(x, y);
    if(object_slot < 0)
        return N3D_RE_DeferredPickup(NULL);

    return N3D_RE_ApplyPickupObject((uint16_t)object_slot);
}
