#include "n3d_re_guard_perception.h"

#include "n3d_re_collision.h"
#include "n3d_re_definitions.h"
#include "n3d_re_door.h"

static int N3D_RE_AbsInt(int value)
{
    return value < 0 ? -value : value;
}

int N3D_RE_GuardPerceptionPrefilter(
    const n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    int ignore_facing)
{
    if(!guard || !object)
        return 0;

    const int guard_tile_x =
        (int)N3D_RE_WorldToTile(object->world_x);
    const int guard_tile_y =
        (int)N3D_RE_WorldToTile(object->world_y);
    const int player_tile_x =
        (int)N3D_RE_WorldToTile(player_world_x);
    const int player_tile_y =
        (int)N3D_RE_WorldToTile(player_world_y);

    const int dx = player_tile_x - guard_tile_x;
    const int dy = player_tile_y - guard_tile_y;
    const int abs_x = N3D_RE_AbsInt(dx);
    const int abs_y = N3D_RE_AbsInt(dy);

    if(abs_x > 8 || abs_y > 8)
        return 0;

    if(ignore_facing)
        return 1;

    const int octant = guard->octant & 7;
    const int facing_mask =
        (1 << ((octant - 1) & 7)) |
        (1 << octant) |
        (1 << ((octant + 1) & 7));

    const int dominant_axis_mask =
        abs_x < abs_y ? 0x99 : 0x66;

    int candidate_mask =
        facing_mask & dominant_axis_mask;

    candidate_mask &=
        dy >= 0 ? 0x3C : 0xC3;

    const int x_side_mask =
        dx >= 0 ? 0x0F : 0xF0;

    return (candidate_mask & x_side_mask) != 0;
}

int N3D_RE_GuardLosCellBlocks(
    uint8_t primary_flags,
    uint8_t secondary_flags,
    int secondary_cell_checks,
    int linked_runtime_record_passable)
{
    if(primary_flags & 0x02)
    {
        if(primary_flags & 0x04)
            return 1;

        if((primary_flags & 0x08) &&
           !linked_runtime_record_passable)
        {
            return 1;
        }
    }

    if(secondary_cell_checks &&
       (secondary_flags & 0x02) &&
       (secondary_flags & 0x20) == 0)
    {
        return 1;
    }

    return 0;
}

int N3D_RE_TraceGuardGridLine(
    int start_x,
    int start_y,
    int delta_x,
    int delta_y,
    int max_steps,
    int secondary_cell_checks,
    n3d_guard_los_block_callback is_intermediate_blocked,
    void* user)
{
    const int step_x = delta_x > 0 ? 1 : -1;
    const int step_y = delta_y > 0 ? 1 : -1;
    const int target_x = start_x + delta_x;
    const int target_y = start_y + delta_y;
    const int abs_x = N3D_RE_AbsInt(delta_x);
    const int abs_y = N3D_RE_AbsInt(delta_y);
    const int x_major = abs_y < abs_x;

    int error;
    int straight_adjust;
    int diagonal_adjust;

    if(x_major)
    {
        error = 2 * abs_y - abs_x;
        straight_adjust = 2 * abs_y;
        diagonal_adjust = 2 * (abs_y - abs_x);
    }
    else
    {
        error = 2 * abs_x - abs_y;
        straight_adjust = 2 * abs_x;
        diagonal_adjust = 2 * (abs_x - abs_y);
    }

    int x = start_x;
    int y = start_y;

    if(max_steps <= 0)
        return 0;

    for(int step = 0; step < max_steps; ++step)
    {
        if(x_major)
            x += step_x;
        else
            y += step_y;

        if(error < 0)
        {
            error += straight_adjust;
        }
        else
        {
            error += diagonal_adjust;
            if(x_major)
                y += step_y;
            else
                x += step_x;
        }

        if(x == target_x && y == target_y)
            return 1;

        if(is_intermediate_blocked &&
           is_intermediate_blocked(
               x,
               y,
               secondary_cell_checks,
               user))
        {
            return 0;
        }
    }

    return 0;
}

int N3D_RE_GuardMapIntermediateBlocked(
    int tile_x,
    int tile_y,
    int secondary_cell_checks,
    void* user)
{
    (void)user;

    if(tile_x < 0 || tile_y < 0 ||
       tile_x >= N3D_MAP_WIDTH ||
       tile_y >= N3D_MAP_HEIGHT)
    {
        return 1;
    }

    const n3d_map_cell* cell =
        N3D_RE_MapCell(
            (uint8_t)tile_x,
            (uint8_t)tile_y);

    if(!cell ||
       !N3D_RE_WallPropertyKnown(cell->wall) ||
       !N3D_RE_ObjectPropertyKnown(cell->object))
    {
        return 1;
    }

    const uint8_t primary_flags =
        n3d_wall_property_resolved[cell->wall];
    const uint8_t secondary_flags =
        n3d_object_property_resolved[cell->object];

    int linked_runtime_record_passable = 1;

    if(primary_flags & 0x08)
    {
        linked_runtime_record_passable =
            N3D_RE_DoorCellStateKnown(
                (uint8_t)tile_x,
                (uint8_t)tile_y) &&
            N3D_RE_DoorCellAllowsPassage(
                (uint8_t)tile_x,
                (uint8_t)tile_y);
    }

    return N3D_RE_GuardLosCellBlocks(
        primary_flags,
        secondary_flags,
        secondary_cell_checks,
        linked_runtime_record_passable);
}

int N3D_RE_EvaluateGuardPerceptionMap(
    const n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    int secondary_cell_checks,
    int ignore_facing)
{
    if(!N3D_RE_GuardPerceptionPrefilter(
           guard,
           object,
           player_world_x,
           player_world_y,
           ignore_facing))
    {
        return 0;
    }

    const int guard_tile_x =
        (int)N3D_RE_WorldToTile(object->world_x);
    const int guard_tile_y =
        (int)N3D_RE_WorldToTile(object->world_y);
    const int player_tile_x =
        (int)N3D_RE_WorldToTile(player_world_x);
    const int player_tile_y =
        (int)N3D_RE_WorldToTile(player_world_y);

    if(guard_tile_x == player_tile_x &&
       guard_tile_y == player_tile_y)
    {
        return 1;
    }

    return N3D_RE_TraceGuardGridLine(
        guard_tile_x,
        guard_tile_y,
        player_tile_x - guard_tile_x,
        player_tile_y - guard_tile_y,
        8,
        secondary_cell_checks,
        N3D_RE_GuardMapIntermediateBlocked,
        NULL);
}

int N3D_RE_TryEvaluateGuardAttackGate(
    n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    int perception_succeeded,
    int* attack_eligible)
{
    if(!guard || !object || !attack_eligible)
        return 0;

    guard->unknown_17 =
        perception_succeeded ? 1 : 0;

    const int dx =
        (int)player_world_x - object->world_x;
    const int dy =
        (int)player_world_y - object->world_y;

    const int within_one_tile =
        N3D_RE_AbsInt(dx) <= N3D_WORLD_UNITS_PER_TILE &&
        N3D_RE_AbsInt(dy) <= N3D_WORLD_UNITS_PER_TILE;

    guard->unknown_18 =
        within_one_tile ? 1 : 0;

    switch(guard->transition_flag)
    {
        case 0:
            *attack_eligible = within_one_tile;
            return 1;

        case 1:
        case 2:
            *attack_eligible =
                perception_succeeded ? 1 : 0;
            return 1;

        default:
            *attack_eligible = 0;
            return 0;
    }
}
