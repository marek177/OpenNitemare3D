#include "n3d_re_combat.h"

int N3D_RE_ScalePlayerDamageByDifficulty(int damage, uint8_t difficulty)
{
    switch(difficulty)
    {
        case 0: return damage * 2;
        case 1: return damage;
        case 2: return damage / 2;
        default: return damage;
    }
}

int N3D_RE_ScaleEnemyDamageByDifficulty(int damage, uint8_t difficulty)
{
    switch(difficulty)
    {
        case 0: return damage / 2;
        case 1: return damage;
        case 2: return damage * 2;
        default: return damage;
    }
}

int N3D_RE_ApplyClassWeaponDamageTransform(
    int raw_damage,
    uint8_t object_class,
    uint8_t weapon_selector,
    uint8_t hamerstein_gate_value)
{
    if(raw_damage <= 0)
        return raw_damage;

    const int wand = weapon_selector == N3D_WEAPON_MAGIC_WAND;
    const int silver = weapon_selector == N3D_WEAPON_SILVER_PISTOL;

    switch(object_class)
    {
        case 0x0C:
        case 0x1D:
            return raw_damage / 8;

        case 0x0D:
            return wand ? raw_damage / 2 : raw_damage / 8;

        case 0x0E:
        case 0x11:
        case 0x14:
            return (wand || silver) ? raw_damage / 2 : raw_damage / 8;

        case 0x0F:
        case 0x10:
            return wand ? raw_damage / 2 : raw_damage / 256;

        case 0x12:
        case 0x13:
            return raw_damage / 4;

        case 0x15:
        case 0x19:
            return 0;

        case 0x16:
            return hamerstein_gate_value == N3D_HAMERSTEIN_GATE_REQUIRED
                ? N3D_HAMERSTEIN_BASE_DAMAGE
                : 0;

        case 0x17:
            return wand ? raw_damage / 256 : raw_damage / 4;

        case 0x18:
            if(wand) return raw_damage / 256;
            if(silver) return raw_damage / 16;
            return raw_damage / 8;

        case 0x1A:
            return wand ? raw_damage / 2 : 0;

        case 0x1B:
        case 0x1C:
            return raw_damage / 2;

        case 0x1E:
            return wand ? 0 : raw_damage / 8;

        case 0x1F:
            return wand ? 0 : raw_damage / 4;

        default:
            return raw_damage;
    }
}

n3d_damage_result N3D_RE_ComputePlayerGuardDamage(
    int16_t projected_base_row,
    int16_t viewport_center_y,
    uint8_t object_class,
    uint8_t weapon_selector,
    uint8_t difficulty,
    uint8_t hamerstein_gate_value,
    uint16_t rng_value)
{
    n3d_damage_result result;

    result.raw_seed =
        8 * ((int)projected_base_row - (int)viewport_center_y) +
        (rng_value % 25);

    result.class_transformed =
        N3D_RE_ApplyClassWeaponDamageTransform(
            result.raw_seed,
            object_class,
            weapon_selector,
            hamerstein_gate_value);

    result.difficulty_transformed =
        N3D_RE_ScalePlayerDamageByDifficulty(
            result.class_transformed,
            difficulty);

    if(result.difficulty_transformed > N3D_MAX_DAMAGE)
        result.difficulty_transformed = N3D_MAX_DAMAGE;

    result.stored_byte = (uint8_t)result.difficulty_transformed;
    return result;
}

n3d_guard_hit_result N3D_RE_ApplyGuardDamage(uint16_t guard_slot, uint8_t damage)
{
    if(guard_slot >= n3d_guard_count)
        return N3D_GUARD_HIT_NO_DAMAGE;

    n3d_guard_record* guard = &n3d_guards[guard_slot];
    if(guard->object_slot >= n3d_object_count)
        return N3D_GUARD_HIT_NO_DAMAGE;

    n3d_object_record* object = &n3d_objects[guard->object_slot];

    if(damage == 0)
        return N3D_GUARD_HIT_NO_DAMAGE;

    if(damage >= guard->strength)
    {
        if(object->object_class == 0x11)
        {
            object->object_class = 0x14;
            guard->strength = N3D_GUARD_FRESH_STRENGTH;
            guard->state = N3D_GUARD_STATE_08;
            guard->next_state = N3D_GUARD_STATE_02;
            guard->timer = 1;
            return N3D_GUARD_HIT_DRACULA_TRANSFORMED;
        }

        guard->strength = 0;
        return N3D_GUARD_HIT_KILLED;
    }

    guard->strength = (uint8_t)(guard->strength - damage);
    guard->result_octant = N3D_GUARD_PAIN_RESULT_OCTANT;

    if(guard->state == N3D_GUARD_STATE_03 ||
       guard->state == N3D_GUARD_STATE_04 ||
       guard->state == N3D_GUARD_STATE_LETHAL_CONTACT ||
       guard->strategy == 3 ||
       guard->strategy == 4 ||
       guard->strategy == 5)
    {
        return N3D_GUARD_HIT_SPECIAL_REACTION_REQUIRED;
    }

    if(guard->state != N3D_GUARD_STATE_00)
        guard->next_state = guard->state;

    guard->state = N3D_GUARD_STATE_PAIN;
    return N3D_GUARD_HIT_PAIN;
}

int N3D_RE_GuardScoreForClass(uint8_t object_class)
{
    static const int score[25] = {
        25, 75, 50, 100, 250,
        150, 200, 100, 100, 0,
        150, 150, 200, -1000, 1000,
        100, 200, 0, 25, 100,
        100, 250, 250, 200, 50
    };

    if(object_class < 0x08 || object_class > 0x20)
        return 0;

    return score[object_class - 0x08];
}
