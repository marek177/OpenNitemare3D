#ifndef N3D_RE_COLLISION_H
#define N3D_RE_COLLISION_H

#include "n3d_re_runtime.h"

#include <stddef.h>
#include <stdint.h>

#define N3D_ENTERED_TILE_EVENT 0x16

typedef struct n3d_byte_table
{
    uint8_t value[256];
} n3d_byte_table;

typedef int (*n3d_door_passable_callback)(uint8_t x, uint8_t y, void* user);
typedef void (*n3d_cell_touch_callback)(uint8_t x, uint8_t y, void* user);

typedef struct n3d_collision_callbacks
{
    n3d_door_passable_callback door_passable;
    n3d_cell_touch_callback wall_script_touch;
    n3d_cell_touch_callback object_touch;
    void* user;
} n3d_collision_callbacks;

typedef struct n3d_post_move_result
{
    int valid;
    int32_t tile_x;
    int32_t tile_y;
    uint16_t map_byte_offset;
    uint8_t entered_tile_event;
} n3d_post_move_result;

uint8_t N3D_RE_WallPropertiesForMappedType(uint8_t type);
uint8_t N3D_RE_ObjectPropertiesForMappedType(uint8_t type);
void N3D_RE_BuildWallProperties(const uint8_t mapped_types[256], n3d_byte_table* out);
void N3D_RE_BuildObjectProperties(const uint8_t mapped_types[256], n3d_byte_table* out);
int N3D_RE_DoorStateAllowsPassage(uint16_t state);
int32_t N3D_RE_WorldToTile(int32_t world);
int N3D_RE_MapCellByteOffset(int32_t tile_x, int32_t tile_y, uint16_t* out_offset);
n3d_post_move_result N3D_RE_PostMoveCell(
    int32_t world_x,
    int32_t world_y,
    int32_t old_tile_x,
    int32_t old_tile_y);

int N3D_RE_TestLeadingEdgePair(
    uint8_t ax, uint8_t ay,
    uint8_t bx, uint8_t by,
    int signed_step,
    const n3d_byte_table* wall_properties,
    const n3d_byte_table* object_properties,
    const n3d_collision_callbacks* callbacks);

#endif
