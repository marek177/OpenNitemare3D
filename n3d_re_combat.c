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

int N3D_RE_GuardAttackUsesRandom(uint8_t object_class)
{
    switch(object_class)
    {
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
            return 1;

        default:
            return 0;
    }
}

int N3D_RE_OriginalRoundedSqrt(int squared_distance)
{
    if(squared_distance <= 1)
        return squared_distance < 0 ? 0 : squared_distance;

    /*
     * Exact semantic port of FUN_1018_324A without depending on libm:
     * floor(sqrt(n)), then round up when remainder >= root-1.
     */
    int root = 0;
    while((root + 1) <= 46340 &&
          (root + 1) * (root + 1) <= squared_distance)
    {
        ++root;
    }

    const int remainder =
        squared_distance - root * root;

    if(remainder >= root - 1)
        ++root;

    return root;
}

int N3D_RE_ComputeGuardAttackDistanceMetric(
    int16_t guard_world_x,
    int16_t guard_world_y,
    int16_t player_world_x,
    int16_t player_world_y)
{
    const int guard_tile_x =
        guard_world_x >> 6;
    const int guard_tile_y =
        guard_world_y >> 6;
    const int player_tile_x =
        player_world_x >> 6;
    const int player_tile_y =
        player_world_y >> 6;

    const int dx =
        guard_tile_x - player_tile_x;
    const int dy =
        guard_tile_y - player_tile_y;

    return N3D_RE_OriginalRoundedSqrt(
        dx * dx + dy * dy);
}

int N3D_RE_ApplyGuardAttackClassTransform(
    int distance_seed,
    uint8_t object_class,
    int class16_full_damage_gate,
    uint16_t rng_value)
{
    switch(object_class)
    {
        case 0x08:
            return rng_value & 0x07;

        case 0x09:
        case 0x0A:
            return rng_value & 0x0F;

        case 0x0B:
            return distance_seed / 4;

        case 0x0C:
        case 0x1D:
        case 0x1E:
            return distance_seed;

        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
            return rng_value & 0x1F;

        case 0x16:
            return class16_full_damage_gate ? 100 : 0x21;

        case 0x19:
            return 100;

        default:
            return distance_seed / 2;
    }
}

n3d_guard_attack_damage_result N3D_RE_ComputeGuardToPlayerDamage(
    int distance_metric,
    uint8_t object_class,
    uint8_t difficulty,
    int class16_full_damage_gate,
    uint16_t rng_value)
{
    n3d_guard_attack_damage_result result;

    const int seed =
        distance_metric > 0
            ? 100 / distance_metric
            : 100;

    result.distance_seed = seed;
    result.class_transformed =
        N3D_RE_ApplyGuardAttackClassTransform(
            seed,
            object_class,
            class16_full_damage_gate,
            rng_value);
    result.difficulty_transformed =
        N3D_RE_ScaleEnemyDamageByDifficulty(
            result.class_transformed,
            difficulty);
    result.stored_byte =
        (uint8_t)result.difficulty_transformed;

    return result;
}

n3d_guard_attack_damage_result
N3D_RE_ComputeGuardToPlayerDamageFromWorld(
    int16_t guard_world_x,
    int16_t guard_world_y,
    int16_t player_world_x,
    int16_t player_world_y,
    uint8_t object_class,
    uint8_t difficulty,
    int class16_full_damage_gate,
    uint16_t rng_value)
{
    return N3D_RE_ComputeGuardToPlayerDamage(
        N3D_RE_ComputeGuardAttackDistanceMetric(
            guard_world_x,
            guard_world_y,
            player_world_x,
            player_world_y),
        object_class,
        difficulty,
        class16_full_damage_gate,
        rng_value);
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
