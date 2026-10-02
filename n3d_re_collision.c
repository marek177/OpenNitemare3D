#include "n3d_re_collision.h"

#include <string.h>

uint8_t N3D_RE_WallPropertiesForMappedType(uint8_t type)
{
    uint8_t flags = 0;

    if (type >= 0x01 && type <= 0x30) flags |= 0x04;
    if (type >= 0x2E && type <= 0x2F) flags |= 0x10;
    if (type >= 0x31 && type <= 0x40) flags |= 0x08;
    if ((flags & (0x04 | 0x08)) != 0) flags |= 0x01;
    if (type >= 0x01 && type <= 0x40) flags |= 0x02;
    if (type >= 0x47 && type <= 0x48) flags |= 0x40;

    return flags;
}

uint8_t N3D_RE_ObjectPropertiesForMappedType(uint8_t type)
{
    uint8_t flags = 0;

    if (type >= 0x06 && type <= 0x3D) flags |= 0x01;
    if (type >= 0x08 && type <= 0x2D) flags |= 0x02;
    if (type >= 0x2F && type <= 0x3D) flags |= 0x04;
    if (type >= 0x08 && type <= 0x25) flags |= 0x08;
    if (type == 0x2A) flags |= 0x20;
    if (type == 0x04) flags |= 0x40;

    return flags;
}

void N3D_RE_BuildWallProperties(const uint8_t mapped_types[256], n3d_byte_table* out)
{
    if (!mapped_types || !out)
        return;

    for (int i = 0; i < 256; ++i)
        out->value[i] = N3D_RE_WallPropertiesForMappedType(mapped_types[i]);
}

void N3D_RE_BuildObjectProperties(const uint8_t mapped_types[256], n3d_byte_table* out)
{
    if (!mapped_types || !out)
        return;

    for (int i = 0; i < 256; ++i)
        out->value[i] = N3D_RE_ObjectPropertiesForMappedType(mapped_types[i]);
}

int N3D_RE_DoorStateAllowsPassage(uint16_t state)
{
    return state == 0 || state == 4;
}

int32_t N3D_RE_WorldToTile(int32_t world)
{
    int32_t q = world / N3D_WORLD_UNITS_PER_TILE;
    int32_t r = world % N3D_WORLD_UNITS_PER_TILE;

    if (world < 0 && r != 0)
        --q;

    return q;
}

int N3D_RE_MapCellByteOffset(int32_t tile_x, int32_t tile_y, uint16_t* out_offset)
{
    if (!out_offset ||
        tile_x < 0 || tile_y < 0 ||
        tile_x >= N3D_MAP_WIDTH || tile_y >= N3D_MAP_HEIGHT)
        return 0;

    *out_offset = (uint16_t)((tile_y * N3D_MAP_WIDTH + tile_x) * N3D_MAP_CELL_BYTES);
    return 1;
}

n3d_post_move_result N3D_RE_PostMoveCell(
    int32_t world_x,
    int32_t world_y,
    int32_t old_tile_x,
    int32_t old_tile_y)
{
    n3d_post_move_result result;
    memset(&result, 0, sizeof(result));

    const int32_t tile_x = N3D_RE_WorldToTile(world_x);
    const int32_t tile_y = N3D_RE_WorldToTile(world_y);

    uint16_t offset = 0;
    if (!N3D_RE_MapCellByteOffset(tile_x, tile_y, &offset))
        return result;

    result.valid = 1;
    result.tile_x = tile_x;
    result.tile_y = tile_y;
    result.map_byte_offset = offset;

    if (tile_x != old_tile_x || tile_y != old_tile_y)
        result.entered_tile_event = N3D_ENTERED_TILE_EVENT;

    return result;
}

int N3D_RE_TestLeadingEdgePair(
    uint8_t ax, uint8_t ay,
    uint8_t bx, uint8_t by,
    int signed_step,
    const n3d_byte_table* wall_properties,
    const n3d_byte_table* object_properties,
    const n3d_collision_callbacks* callbacks)
{
    if (!wall_properties || !object_properties)
        return 0;

    const n3d_map_cell* a = N3D_RE_MapCell(ax, ay);
    const n3d_map_cell* b = N3D_RE_MapCell(bx, by);
    if (!a || !b)
        return 0;

    const uint8_t wa = wall_properties->value[a->wall];
    const uint8_t wb = wall_properties->value[b->wall];

    if ((wa & 0x04) || (wb & 0x04))
        return 0;

    if (wa & 0x08)
    {
        if (!callbacks || !callbacks->door_passable ||
            !callbacks->door_passable(ax, ay, callbacks->user))
            return 0;
    }

    if (wb & 0x08)
    {
        if (!callbacks || !callbacks->door_passable ||
            !callbacks->door_passable(bx, by, callbacks->user))
            return 0;
    }

    if (callbacks && callbacks->wall_script_touch)
    {
        if (wa & 0x40) callbacks->wall_script_touch(ax, ay, callbacks->user);
        if (wb & 0x40) callbacks->wall_script_touch(bx, by, callbacks->user);
    }

    const uint8_t oa = object_properties->value[a->object];
    const uint8_t ob = object_properties->value[b->object];

    if (callbacks && callbacks->object_touch)
    {
        if (oa & 0x04) callbacks->object_touch(ax, ay, callbacks->user);
        if (ob & 0x04) callbacks->object_touch(bx, by, callbacks->user);
    }

    if ((oa & 0x02) || (ob & 0x02))
        return 0;

    return signed_step;
}
