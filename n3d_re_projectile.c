#include "n3d_re_projectile.h"

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
