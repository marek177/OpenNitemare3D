#include "n3d_re_pickup.h"

#include <string.h>

static n3d_pickup_result N3D_RE_DeferredPickup(
    const n3d_object_record* object)
{
    n3d_pickup_result result = {0};
    if(object)
    {
        result.kind = N3D_PICKUP_DEFERRED;
        result.object_class = object->object_class;
        result.map_object_id = object->map_object_id;
        result.variant_index = object->variant;
    }
    else
    {
        result.kind = N3D_PICKUP_UNRESOLVED;
    }
    return result;
}

static n3d_pickup_result N3D_RE_InactivePickup(
    const n3d_object_record* object)
{
    n3d_pickup_result result = {0};
    result.kind = N3D_PICKUP_INACTIVE;

    if(object)
    {
        result.object_class = object->object_class;
        result.map_object_id = object->map_object_id;
        result.variant_index = object->variant;
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

int N3D_RE_PickupResultAccepted(const n3d_pickup_result* result)
{
    return result && result->accepted != 0;
}

void N3D_RE_DeactivateAcceptedPickup(uint16_t object_slot)
{
    if(object_slot >= n3d_object_count)
        return;

    /*
     * CF4A removal clears the active/runtime-present flag. Keep the MAP byte
     * unchanged here until its exact generic pickup store is isolated; the
     * inactive OBJECT is enough to make repeated one-unit touch callbacks no-op.
     */
    n3d_objects[object_slot].flags =
        (uint8_t)(n3d_objects[object_slot].flags &
                  (uint8_t)~N3D_OBJECT_RUNTIME_PRESENT);
}

n3d_pickup_result N3D_RE_ApplyPickupObject(uint16_t object_slot)
{
    if(object_slot >= n3d_object_count)
        return N3D_RE_DeferredPickup(NULL);

    n3d_object_record* object = &n3d_objects[object_slot];

    if((object->flags & N3D_OBJECT_RUNTIME_PRESENT) == 0)
        return N3D_RE_InactivePickup(object);

    n3d_pickup_result result = {0};
    result.object_class = object->object_class;
    result.map_object_id = object->map_object_id;
    result.variant_index = object->variant;

    switch(object->object_class)
    {
        case 0x2F: /* KEY: subtype selects colored-key bit */
            if(object->variant >= 4)
                return N3D_RE_DeferredPickup(object);

            N3D_RE_GrantInventoryBit(
                &n3d_player.colored_keys,
                object->variant);
            result.kind = N3D_PICKUP_KEY_GRANTED;
            result.accepted = 1;
            break;

        case 0x30: /* IDCARD: subtype 0/1 */
            if(object->variant >= 2)
                return N3D_RE_DeferredPickup(object);

            N3D_RE_GrantInventoryBit(
                &n3d_player.id_cards,
                object->variant);
            result.kind = N3D_PICKUP_IDCARD_GRANTED;
            result.accepted = 1;
            break;

        case 0x3C: /* PENTAGRAM: subtype selects progress bit */
            if(object->variant >= 4)
                return N3D_RE_DeferredPickup(object);

            N3D_RE_GrantInventoryBit(
                &n3d_player.pentagrams,
                object->variant);
            result.kind = N3D_PICKUP_PENTAGRAM_GRANTED;
            result.accepted = 1;
            break;

        case 0x39: /* AMMO: subtype 0=silver, 1=laser/plasma, 2=wand */
        {
            uint8_t* ammo = NULL;

            switch(object->variant)
            {
                case 0:
                    result.ammo_pool = N3D_AMMO_POOL_SILVER;
                    ammo = &n3d_player.silver_ammo;
                    break;

                case 1:
                    result.ammo_pool = N3D_AMMO_POOL_LASER;
                    ammo = &n3d_player.laser_ammo;
                    break;

                case 2:
                    result.ammo_pool = N3D_AMMO_POOL_WAND;
                    ammo = &n3d_player.wand_ammo;
                    break;

                default:
                    return N3D_RE_DeferredPickup(object);
            }

            result.value_before = *ammo;
            if(N3D_RE_AddAmmoPickup(ammo))
            {
                result.value_after = *ammo;
                result.kind = N3D_PICKUP_AMMO_ADDED;
                result.accepted = 1;
            }
            else
            {
                result.value_after = *ammo;
                result.kind = N3D_PICKUP_AMMO_AT_THRESHOLD;
                result.accepted = 0;
            }
            break;
        }

        default:
            return N3D_RE_DeferredPickup(object);
    }

    if(result.accepted)
        N3D_RE_DeactivateAcceptedPickup(object_slot);

    return result;
}

n3d_pickup_result N3D_RE_ApplyPickupAtCell(uint8_t x, uint8_t y)
{
    const int object_slot = N3D_RE_FindObjectSlotByCell(x, y);
    if(object_slot < 0)
        return N3D_RE_DeferredPickup(NULL);

    return N3D_RE_ApplyPickupObject((uint16_t)object_slot);
}

void N3D_RE_PlayerPickupTouchCallback(uint8_t x, uint8_t y, void* user)
{
    n3d_pickup_result result = N3D_RE_ApplyPickupAtCell(x, y);

    if(!user)
        return;

    n3d_pickup_touch_context* context =
        (n3d_pickup_touch_context*)user;

    context->last_result = result;
    ++context->touch_calls;
    if(result.accepted)
        ++context->accepted_pickups;
}
