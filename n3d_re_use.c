#include "n3d_re_use.h"

#include "n3d_re_definitions.h"
#include "n3d_re_runtime.h"

static uint8_t n3d_use_previous;

void N3D_RE_ResetUseLatch(void)
{
    n3d_use_previous = 0;
}

int N3D_RE_UseRisingEdge(int use_down)
{
    const uint8_t current = use_down ? 1 : 0;
    const int rising = current && !n3d_use_previous;
    n3d_use_previous = current;
    return rising;
}

int N3D_RE_AdjacentUseCell(
    uint8_t player_x,
    uint8_t player_y,
    uint8_t octant,
    uint8_t* target_x,
    uint8_t* target_y)
{
    if(!target_x || !target_y)
        return 0;

    int x = player_x;
    int y = player_y;

    switch(octant & 7u)
    {
        case 0:
        case 7:
            --y;
            break;

        case 1:
        case 2:
            ++x;
            break;

        case 3:
        case 4:
            ++y;
            break;

        case 5:
        case 6:
            --x;
            break;
    }

    /*
     * Original code operates on the adjacent MAP pointer. The modern rewrite
     * rejects out-of-map targets explicitly rather than permitting wraparound.
     */
    if(x < 0 || y < 0 || x >= N3D_MAP_WIDTH || y >= N3D_MAP_HEIGHT)
        return 0;

    *target_x = (uint8_t)x;
    *target_y = (uint8_t)y;
    return 1;
}

n3d_use_target N3D_RE_ClassifyUseTarget(uint8_t octant)
{
    n3d_use_target target = {0};

    uint8_t x = 0, y = 0;
    if(!N3D_RE_AdjacentUseCell(
            (uint8_t)n3d_player.tile_x,
            (uint8_t)n3d_player.tile_y,
            octant,
            &x,
            &y))
    {
        target.kind = N3D_USE_UNRESOLVED;
        return target;
    }

    target.x = x;
    target.y = y;

    const n3d_map_cell* cell = N3D_RE_MapCell(x, y);
    if(!cell)
    {
        target.kind = N3D_USE_UNRESOLVED;
        return target;
    }

    target.raw_wall_id = cell->wall;
    target.raw_object_id = cell->object;

    if(!N3D_RE_WallPropertyKnown(cell->wall) ||
       !N3D_RE_ObjectPropertyKnown(cell->object))
    {
        target.kind = N3D_USE_UNRESOLVED;
        return target;
    }

    if(N3D_RE_WallMappingKnown(cell->wall))
        target.mapped_wall_type = n3d_wall_mapped_type[cell->wall];

    if(N3D_RE_ObjectMappingKnown(cell->object))
        target.mapped_object_type = n3d_object_mapped_type[cell->object];

    if(n3d_wall_property_resolved[cell->wall] & 0x08)
    {
        target.kind = N3D_USE_DYNAMIC_DOOR;
        return target;
    }

    if(!N3D_RE_WallMappingKnown(cell->wall))
    {
        target.kind = N3D_USE_UNRESOLVED;
        return target;
    }

    if(target.mapped_wall_type != 0)
    {
        target.kind = N3D_USE_MAPPED_WALL;
        return target;
    }

    if(cell->object != 0 && !N3D_RE_ObjectMappingKnown(cell->object))
    {
        target.kind = N3D_USE_UNRESOLVED;
        return target;
    }

    switch(target.mapped_object_type)
    {
        case 0:
            target.kind = N3D_USE_NONE;
            break;

        case 0x03:
            target.kind = N3D_USE_PANEL;
            break;

        case 0x28:
            target.kind = N3D_USE_PUSH;
            break;

        default:
            target.kind = N3D_USE_MAPPED_OBJECT;
            break;
    }

    return target;
}
