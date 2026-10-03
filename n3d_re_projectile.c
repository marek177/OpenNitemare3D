#include "n3d_re_projectile.h"
#include "n3d_re_definitions.h"
#include "n3d_re_collision.h"

#include <string.h>

int N3D_RE_WeaponUsesProjectile(uint8_t weapon_selector)
{
    return weapon_selector == N3D_WEAPON_SINGLE_LASER ||
           weapon_selector == N3D_WEAPON_MAGIC_WAND ||
           weapon_selector == N3D_WEAPON_CONTINUOUS_LASER;
}

int N3D_RE_ProjectileSequenceOffsets(
    uint8_t weapon_selector,
    n3d_projectile_sequence_offsets* out)
{
    if (!out)
        return 0;

    switch(weapon_selector)
    {
        case N3D_WEAPON_SINGLE_LASER:
        case N3D_WEAPON_CONTINUOUS_LASER:
            out->flight = 0;
            out->impact = 1;
            return 1;

        case N3D_WEAPON_MAGIC_WAND:
            out->flight = 2;
            out->impact = 3;
            return 1;

        default:
            return 0;
    }
}

int N3D_RE_FirstFreeProjectileSlot(void)
{
    for(int i = 0; i < N3D_MAX_PROJECTILES; ++i)
    {
        if(n3d_projectiles[i].state == 0)
            return i;
    }

    return -1;
}

int N3D_RE_ProjectileHitsGuard(
    int32_t projectile_x,
    int32_t projectile_y,
    int32_t guard_x,
    int32_t guard_y)
{
    int64_t dx = (int64_t)projectile_x - guard_x;
    int64_t dy = (int64_t)projectile_y - guard_y;

    return dx >= -N3D_PROJECTILE_GUARD_HIT_TOLERANCE &&
           dx <=  N3D_PROJECTILE_GUARD_HIT_TOLERANCE &&
           dy >= -N3D_PROJECTILE_GUARD_HIT_TOLERANCE &&
           dy <=  N3D_PROJECTILE_GUARD_HIT_TOLERANCE;
}

int N3D_RE_ProjectileNeedsProjection(
    int32_t projectile_x,
    int32_t projectile_y,
    int32_t player_x,
    int32_t player_y)
{
    int64_t dx = (int64_t)projectile_x - player_x;
    int64_t dy = (int64_t)projectile_y - player_y;

    return dx > N3D_PROJECTILE_RENDER_DISTANCE_THRESHOLD ||
           dx < -N3D_PROJECTILE_RENDER_DISTANCE_THRESHOLD ||
           dy > N3D_PROJECTILE_RENDER_DISTANCE_THRESHOLD ||
           dy < -N3D_PROJECTILE_RENDER_DISTANCE_THRESHOLD;
}

int N3D_RE_InitializeProjectile(
    int slot,
    uint8_t weapon_selector,
    int16_t world_x,
    int16_t world_y,
    uint8_t sequence_base)
{
    if(slot < 0 || slot >= N3D_MAX_PROJECTILES)
        return 0;

    n3d_projectile_sequence_offsets seq;
    if(!N3D_RE_ProjectileSequenceOffsets(weapon_selector, &seq))
        return 0;

    n3d_projectile_record* projectile = &n3d_projectiles[slot];
    memset(projectile, 0, sizeof(*projectile));

    projectile->state = 1;
    projectile->object.animation_frame = 0;
    projectile->object.sequence_id = (uint8_t)(sequence_base + seq.flight);
    projectile->object.flags = N3D_OBJECT_RUNTIME_PRESENT;
    projectile->object.object_class = 5;
    projectile->object.world_x = world_x;
    projectile->object.world_y = world_y;
    projectile->object.runtime_1a = 5;

    return 1;
}

int N3D_RE_InitializeProjectileFromAngle(
    int slot,
    uint8_t weapon_selector,
    int16_t world_x,
    int16_t world_y,
    uint8_t sequence_base,
    int angle_degrees)
{
    if(!N3D_RE_InitializeProjectile(
            slot,
            weapon_selector,
            world_x,
            world_y,
            sequence_base))
        return 0;

    n3d_line_state line = {0};
    if(!N3D_RE_InitLineStateFromAngle(angle_degrees, &line))
    {
        n3d_projectiles[slot].state = 0;
        return 0;
    }

    n3d_projectile_record* projectile = &n3d_projectiles[slot];
    projectile->x_is_major_axis = line.axis_flag;
    projectile->line_error = line.error;
    projectile->minor_error_step = line.twice_minor;
    projectile->major_error_fixup = line.twice_minor_minus_major;
    projectile->step_x = line.step_x;
    projectile->step_y = line.step_y;

    return 1;
}

int N3D_RE_AdvanceProjectileLineSubstep(int slot)
{
    if(slot < 0 || slot >= N3D_MAX_PROJECTILES)
        return 0;

    n3d_projectile_record* projectile = &n3d_projectiles[slot];
    if(projectile->state != 1)
        return 0;

    /*
     * Exact 9D30-style line stepping over the projectile record itself.
     * Collision/impact decisions are intentionally outside this primitive.
     */
    if(projectile->x_is_major_axis != 0)
        projectile->object.world_x =
            (int16_t)(projectile->object.world_x + projectile->step_x);
    else
        projectile->object.world_y =
            (int16_t)(projectile->object.world_y + projectile->step_y);

    if(projectile->line_error < 0)
    {
        projectile->line_error =
            (int16_t)(
                projectile->line_error +
                projectile->minor_error_step);
    }
    else
    {
        projectile->line_error =
            (int16_t)(
                projectile->line_error +
                projectile->major_error_fixup);

        if(projectile->x_is_major_axis != 0)
            projectile->object.world_y =
                (int16_t)(projectile->object.world_y + projectile->step_y);
        else
            projectile->object.world_x =
                (int16_t)(projectile->object.world_x + projectile->step_x);
    }

    return 1;
}

int N3D_RE_AdvanceProjectileLineSteps(int slot, uint16_t substeps)
{
    int advanced = 0;
    for(uint16_t i = 0; i < substeps; ++i)
    {
        if(!N3D_RE_AdvanceProjectileLineSubstep(slot))
            break;
        ++advanced;
    }
    return advanced;
}

n3d_projectile_collision_result
N3D_RE_ClassifyProjectileCollision(int slot)
{
    n3d_projectile_collision_result result = {0};
    result.object_slot = -1;
    result.guard_slot = -1;

    if(slot < 0 || slot >= N3D_MAX_PROJECTILES)
    {
        result.kind = N3D_PROJECTILE_COLLISION_UNRESOLVED;
        return result;
    }

    const n3d_projectile_record* projectile = &n3d_projectiles[slot];
    if(projectile->state != 1)
        return result;

    const int tile_x =
        (int)N3D_RE_WorldToTile(projectile->object.world_x);
    const int tile_y =
        (int)N3D_RE_WorldToTile(projectile->object.world_y);

    if(tile_x < 0 || tile_y < 0 ||
       tile_x >= N3D_MAP_WIDTH || tile_y >= N3D_MAP_HEIGHT)
    {
        result.kind = N3D_PROJECTILE_COLLISION_UNRESOLVED;
        return result;
    }

    result.cell_x = (uint8_t)tile_x;
    result.cell_y = (uint8_t)tile_y;

    const n3d_map_cell* cell =
        N3D_RE_MapCell(result.cell_x, result.cell_y);
    if(!cell)
    {
        result.kind = N3D_PROJECTILE_COLLISION_UNRESOLVED;
        return result;
    }

    result.raw_wall_id = cell->wall;
    result.raw_object_id = cell->object;

    if(!N3D_RE_WallPropertyKnown(cell->wall) ||
       !N3D_RE_ObjectPropertyKnown(cell->object))
    {
        result.kind = N3D_PROJECTILE_COLLISION_UNRESOLVED;
        return result;
    }

    if(N3D_RE_WallMappingKnown(cell->wall))
        result.mapped_wall_type = n3d_wall_mapped_type[cell->wall];

    /*
     * FUN_1010_9B64 first uses the visited cell's object property/class to
     * select the GUARD path, then applies the +/-9 world-coordinate test.
     */
    if(n3d_object_property_resolved[cell->object] &
       N3D_OBJECT_CREATES_GUARD)
    {
        const int object_slot =
            N3D_RE_FindObjectSlotByCell(result.cell_x, result.cell_y);

        if(object_slot >= 0)
        {
            const n3d_object_record* object =
                &n3d_objects[object_slot];

            const int guard_slot = object->guard_index;
            if(guard_slot >= 0 &&
               guard_slot < (int)n3d_guard_count &&
               n3d_guards[guard_slot].strength != 0 &&
               N3D_RE_ProjectileHitsGuard(
                   projectile->object.world_x,
                   projectile->object.world_y,
                   object->world_x,
                   object->world_y))
            {
                result.object_slot = (int16_t)object_slot;
                result.guard_slot = (int16_t)guard_slot;
                result.kind = N3D_PROJECTILE_COLLISION_GUARD_HIT;
                result.enter_impact = 1;
                return result;
            }
        }
    }

    const uint8_t wall_flags =
        n3d_wall_property_resolved[cell->wall];

    if(wall_flags & 0x10)
    {
        /*
         * Explodable-wall path is verified: event 0x29 and transition toward
         * runtime wall class 0x2D. Final map/collision cleanup stays deferred.
         */
        result.kind = N3D_PROJECTILE_COLLISION_EXPLODABLE_WALL;
        result.event_id = N3D_EXPLODABLE_WALL_EVENT;
        result.enter_impact = 1;
        return result;
    }

    /*
     * Other collision-relevant wall classes are intentionally not assigned
     * cleanup/state effects here until the remaining 9B64 branches are closed.
     */
    if(wall_flags & (0x01 | 0x02 | 0x04 | 0x08))
    {
        result.kind = N3D_PROJECTILE_COLLISION_WALL_DEFERRED;
        return result;
    }

    result.kind = N3D_PROJECTILE_COLLISION_NONE;
    return result;
}

n3d_projectile_advance_result
N3D_RE_AdvanceProjectileUntilCollision(
    int slot,
    uint16_t substeps)
{
    n3d_projectile_advance_result result = {0};
    result.requested_substeps = substeps;
    result.collision.object_slot = -1;
    result.collision.guard_slot = -1;

    if(slot < 0 || slot >= N3D_MAX_PROJECTILES)
    {
        result.collision.kind =
            N3D_PROJECTILE_COLLISION_UNRESOLVED;
        return result;
    }

    for(uint16_t i = 0; i < substeps; ++i)
    {
        if(!N3D_RE_AdvanceProjectileLineSubstep(slot))
            break;

        ++result.advanced_substeps;
        result.collision =
            N3D_RE_ClassifyProjectileCollision(slot);

        if(result.collision.kind !=
           N3D_PROJECTILE_COLLISION_NONE)
        {
            /*
             * Match the recovered 9D30 -> 9B64 ordering: collision is checked
             * after every small movement step and traversal stops immediately.
             * Damage/impact/wall cleanup remain caller-owned.
             */
            break;
        }
    }

    return result;
}

int N3D_RE_EnterProjectileImpact(
    int slot,
    uint8_t weapon_selector,
    uint8_t sequence_base)
{
    if(slot < 0 || slot >= N3D_MAX_PROJECTILES)
        return 0;

    n3d_projectile_sequence_offsets seq;
    if(!N3D_RE_ProjectileSequenceOffsets(weapon_selector, &seq))
        return 0;

    n3d_projectile_record* projectile = &n3d_projectiles[slot];
    projectile->state = 2;
    projectile->object.animation_frame = 0;
    projectile->object.sequence_id = (uint8_t)(sequence_base + seq.impact);
    projectile->object.flags |= 0x10;

    return 1;
}

int N3D_RE_EnterProjectileImpactFromFlight(int slot)
{
    if(slot < 0 || slot >= N3D_MAX_PROJECTILES)
        return 0;

    n3d_projectile_record* projectile = &n3d_projectiles[slot];
    if(projectile->state != 1)
        return 0;

    /*
     * Every recovered projectile weapon uses impact sequence = flight + 1:
     * 0/3: 0 -> 1, Wand: 2 -> 3.
     */
    projectile->state = 2;
    projectile->object.animation_frame = 0;
    projectile->object.sequence_id =
        (uint8_t)(projectile->object.sequence_id + 1);
    projectile->object.flags |= 0x10;
    return 1;
}

n3d_projectile_guard_resolution
N3D_RE_ResolveProjectileGuardHit(
    int slot,
    const n3d_projectile_collision_result* collision,
    int16_t view_reference_y,
    uint8_t hamerstein_gate_value,
    uint16_t rng_value)
{
    n3d_projectile_guard_resolution result = {0};

    if(!collision ||
       collision->kind != N3D_PROJECTILE_COLLISION_GUARD_HIT ||
       collision->guard_slot < 0 ||
       collision->object_slot < 0 ||
       collision->guard_slot >= (int16_t)n3d_guard_count ||
       collision->object_slot >= (int16_t)n3d_object_count ||
       slot < 0 || slot >= N3D_MAX_PROJECTILES)
    {
        return result;
    }

    if(n3d_player.active_weapon >= N3D_WEAPON_COUNT)
        return result;

    const uint16_t guard_slot = (uint16_t)collision->guard_slot;
    const uint16_t object_slot = (uint16_t)collision->object_slot;
    const uint8_t object_class_before =
        n3d_objects[object_slot].object_class;

    const n3d_damage_result damage =
        N3D_RE_ComputePlayerGuardDamage(
            n3d_objects[object_slot].projected_y_base,
            view_reference_y,
            object_class_before,
            n3d_player.active_weapon,
            n3d_player.difficulty,
            hamerstein_gate_value,
            rng_value);

    /*
     * Original receiver ignores non-positive signed damage. Do not allow the
     * stored byte representation of a negative intermediate to wrap positive.
     */
    const uint8_t applied_damage =
        damage.difficulty_transformed > 0
            ? damage.stored_byte
            : 0;

    result.resolved = 1;
    result.applied_damage = applied_damage;
    result.guard_result =
        N3D_RE_ApplyGuardDamage(guard_slot, applied_damage);

    if(result.guard_result == N3D_GUARD_HIT_KILLED)
        result.score_delta =
            N3D_RE_GuardScoreForClass(object_class_before);

    if(N3D_RE_EnterProjectileImpactFromFlight(slot))
        result.entered_impact = 1;

    return result;
}

void N3D_RE_AdvanceProjectileAnimation(int slot, int frame_count)
{
    if(slot < 0 || slot >= N3D_MAX_PROJECTILES || frame_count <= 0)
        return;

    n3d_projectile_record* projectile = &n3d_projectiles[slot];
    if(projectile->state == 0)
        return;

    int next_frame = projectile->object.animation_frame + 1;

    if(projectile->state == 1)
    {
        if(next_frame >= frame_count)
            next_frame = 0;

        projectile->object.animation_frame = (int8_t)next_frame;
        return;
    }

    if(projectile->state == 2)
    {
        if(next_frame >= frame_count)
        {
            projectile->state = 0;
            return;
        }

        projectile->object.animation_frame = (int8_t)next_frame;
    }
}
