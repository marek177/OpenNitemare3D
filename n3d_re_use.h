#ifndef N3D_RE_USE_H
#define N3D_RE_USE_H

#include "n3d_re_player.h"

#include <stdint.h>

typedef enum n3d_use_target_kind
{
    N3D_USE_NONE = 0,
    N3D_USE_UNRESOLVED,
    N3D_USE_DYNAMIC_DOOR,
    N3D_USE_MAPPED_WALL,
    N3D_USE_PANEL,
    N3D_USE_PUSH,
    N3D_USE_MAPPED_OBJECT
} n3d_use_target_kind;

typedef struct n3d_use_target
{
    n3d_use_target_kind kind;
    uint8_t x;
    uint8_t y;
    uint8_t raw_wall_id;
    uint8_t raw_object_id;
    uint8_t mapped_wall_type;
    uint8_t mapped_object_type;
} n3d_use_target;

void N3D_RE_ResetUseLatch(void);
int N3D_RE_UseRisingEdge(int use_down);

int N3D_RE_AdjacentUseCell(
    uint8_t player_x,
    uint8_t player_y,
    uint8_t octant,
    uint8_t* target_x,
    uint8_t* target_y);

n3d_use_target N3D_RE_ClassifyUseTarget(uint8_t octant);

#endif
