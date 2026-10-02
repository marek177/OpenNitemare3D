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

        case 0x31: /* score-only pickup */
            n3d_player.score += 200;
            result.kind = N3D_PICKUP_SCORE_ADDED;
            result.score_delta = 200;
            result.accepted = 1;
            break;

        case 0x32:
            /*
             * Special/panel charge writer is known, including a 99 limit, but
             * the exact edge write when the shifted increment crosses that
             * limit remains intentionally deferred in this C port.
             */
            return N3D_RE_DeferredPickup(object);

        case 0x33: /* FOOD / health: + (20 >> subtype), gated by HP < 100 */
        {
            if(n3d_player.health >= N3D_PLAYER_MAX_HEALTH)
            {
                result.kind = N3D_PICKUP_HEALTH_AT_THRESHOLD;
                result.value_before = n3d_player.health;
                result.value_after = n3d_player.health;
                break;
            }

            if(object->variant >= 8)
                return N3D_RE_DeferredPickup(object);

            const uint8_t amount = (uint8_t)(20u >> object->variant);
            if(amount == 0)
                return N3D_RE_DeferredPickup(object);

            result.value_before = n3d_player.health;
            n3d_player.health =
                (uint8_t)(n3d_player.health + amount);
            result.value_after = n3d_player.health;
            result.kind = N3D_PICKUP_HEALTH_ADDED;
            result.accepted = 1;
            break;
        }

        case 0x34: /* +30 HP and +250 score, only when HP < 100 */
            if(n3d_player.health >= N3D_PLAYER_MAX_HEALTH)
            {
                result.kind = N3D_PICKUP_HEALTH_AT_THRESHOLD;
                result.value_before = n3d_player.health;
                result.value_after = n3d_player.health;
                break;
            }

            result.value_before = n3d_player.health;
            n3d_player.health =
                (uint8_t)(n3d_player.health + 30);
            result.value_after = n3d_player.health;
            n3d_player.score += 250;
            result.score_delta = 250;
            result.kind = N3D_PICKUP_HEALTH_ADDED;
            result.accepted = 1;
            break;

        case 0x35: /* restore HP/plasma, +500 score, increment 4C1E */
            n3d_player.health = N3D_PLAYER_MAX_HEALTH;
            n3d_player.laser_ammo = N3D_NORMAL_AMMO_CAP;
            n3d_player.score += 500;
            ++n3d_player.pickup_counter_4c1e;
            result.kind = N3D_PICKUP_RESTORE_BONUS;
            result.score_delta = 500;
            result.accepted = 1;
            break;

        case 0x36: /* WEAPON: subtype is runtime weapon selector 0..3 */
            if(!N3D_RE_QueueOwnedWeapon(object->variant))
                return N3D_RE_DeferredPickup(object);

            result.kind = N3D_PICKUP_WEAPON_GRANTED;
            result.accepted = 1;
            break;

        case 0x37:
            /* 4C2B writer is known but its subtype operation stays deferred. */
            return N3D_RE_DeferredPickup(object);

        case 0x38: /* secondary resource 4C21: +20 when below 100 */
            result.value_before = n3d_player.resource_4c21;
            if(n3d_player.resource_4c21 >= N3D_NORMAL_AMMO_CAP)
            {
                result.value_after = n3d_player.resource_4c21;
                result.kind = N3D_PICKUP_RESOURCE_AT_THRESHOLD;
                break;
            }

            n3d_player.resource_4c21 =
                (uint8_t)(n3d_player.resource_4c21 + 20);
            result.value_after = n3d_player.resource_4c21;
            result.kind = N3D_PICKUP_RESOURCE_ADDED;
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
            }
            break;
        }

        case 0x3A: /* Crystal Ball charge */
            result.value_before = n3d_player.crystal_ball_charge;
            if(n3d_player.crystal_ball_charge >= N3D_NORMAL_AMMO_CAP)
            {
                result.value_after = n3d_player.crystal_ball_charge;
                result.kind = N3D_PICKUP_CRYSTAL_CHARGE_AT_THRESHOLD;
                break;
            }

            n3d_player.crystal_ball_charge =
                (uint8_t)(n3d_player.crystal_ball_charge + 20);
            result.value_after = n3d_player.crystal_ball_charge;
            result.kind = N3D_PICKUP_CRYSTAL_CHARGE_ADDED;
            result.accepted = 1;
            break;

        case 0x3B: /* Magic Eye charge */
            result.value_before = n3d_player.magic_eye_charge;
            if(n3d_player.magic_eye_charge >= N3D_NORMAL_AMMO_CAP)
            {
                result.value_after = n3d_player.magic_eye_charge;
                result.kind = N3D_PICKUP_EYE_CHARGE_AT_THRESHOLD;
                break;
            }

            n3d_player.magic_eye_charge =
                (uint8_t)(n3d_player.magic_eye_charge + 20);
            result.value_after = n3d_player.magic_eye_charge;
            result.kind = N3D_PICKUP_EYE_CHARGE_ADDED;
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

        case 0x3D:
            /* Script/scroll helper is a separate subsystem. */
            return N3D_RE_DeferredPickup(object);

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
