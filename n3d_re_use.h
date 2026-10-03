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

typedef enum n3d_use_execution_kind
{
    N3D_USE_EXEC_NONE = 0,
    N3D_USE_EXEC_UNRESOLVED,
    N3D_USE_EXEC_PANEL_ACTIVATED,
    N3D_USE_EXEC_PUSH_STARTED,
    N3D_USE_EXEC_PUSH_BLOCKED,
    N3D_USE_EXEC_DOOR_DEFERRED,
    N3D_USE_EXEC_LEVEL_CHANGE_REQUEST,
    N3D_USE_EXEC_KEY_GATE_PASSED,
    N3D_USE_EXEC_KEY_GATE_BLOCKED,
    N3D_USE_EXEC_PENTAGRAM_GATE_PASSED,
    N3D_USE_EXEC_PENTAGRAM_GATE_BLOCKED,
    N3D_USE_EXEC_REMOTE_TERMINAL_PASSED,
    N3D_USE_EXEC_REMOTE_TERMINAL_BLOCKED,
    N3D_USE_EXEC_SCRIPTED_WALL_REQUEST,
    N3D_USE_EXEC_CLIMB_MENU_REQUEST,
    N3D_USE_EXEC_OTHER_SIDE_REQUEST,
    N3D_USE_EXEC_FLOOR_MENU_REQUEST,
    N3D_USE_EXEC_GO_DOWN_MENU_REQUEST,
    N3D_USE_EXEC_WALL_DEFERRED,
    N3D_USE_EXEC_OBJECT_DEFERRED
} n3d_use_execution_kind;

typedef struct n3d_use_execution
{
    n3d_use_execution_kind kind;
    n3d_use_target target;
    uint8_t event_id;
    uint8_t level_delta;
    uint8_t required_inventory_bit;
    uint8_t menu_first_wall_type;
    uint8_t menu_last_wall_type;
    uint8_t menu_variant_index;
    int runtime_slot;
} n3d_use_execution;

void N3D_RE_ResetUseLatch(void);
int N3D_RE_UseRisingEdge(int use_down);

int N3D_RE_AdjacentUseCell(
    uint8_t player_x,
    uint8_t player_y,
    uint8_t octant,
    uint8_t* target_x,
    uint8_t* target_y);

n3d_use_target N3D_RE_ClassifyUseTarget(uint8_t octant);
n3d_use_execution N3D_RE_ExecuteUse(uint8_t octant);

#endif
