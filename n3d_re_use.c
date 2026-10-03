#include "n3d_re_use.h"

#include "n3d_re_definitions.h"
#include "n3d_re_runtime.h"
#include "n3d_re_special_runtime.h"
#include "n3d_re_door.h"

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


n3d_use_execution N3D_RE_ExecuteUse(uint8_t octant)
{
    n3d_use_execution execution = {0};
    execution.runtime_slot = -1;
    execution.target = N3D_RE_ClassifyUseTarget(octant);

    switch(execution.target.kind)
    {
        case N3D_USE_NONE:
            execution.kind = N3D_USE_EXEC_NONE;
            return execution;

        case N3D_USE_UNRESOLVED:
            execution.kind = N3D_USE_EXEC_UNRESOLVED;
            return execution;

        case N3D_USE_DYNAMIC_DOOR:
            /*
             * Target and runtime slot are known, but the initial/ordinary door
             * transition state machine is not fully closed yet.
             */
            execution.runtime_slot =
                N3D_RE_FindDoorSlotByCell(
                    execution.target.x,
                    execution.target.y);
            execution.kind =
                execution.runtime_slot >= 0
                    ? N3D_USE_EXEC_DOOR_DEFERRED
                    : N3D_USE_EXEC_UNRESOLVED;
            return execution;

        case N3D_USE_PANEL:
        {
            const int panel_slot =
                N3D_RE_FindPanelSlotByCell(
                    execution.target.x,
                    execution.target.y);
            if(panel_slot < 0)
            {
                execution.kind = N3D_USE_EXEC_UNRESOLVED;
                return execution;
            }

            N3D_RE_ActivatePanelUse(&n3d_panels[panel_slot]);
            execution.runtime_slot = panel_slot;
            execution.event_id = N3D_PANEL_USE_EVENT;
            execution.kind = N3D_USE_EXEC_PANEL_ACTIVATED;
            return execution;
        }

        case N3D_USE_PUSH:
        {
            const int object_slot =
                N3D_RE_FindObjectSlotByCell(
                    execution.target.x,
                    execution.target.y);
            if(object_slot < 0)
            {
                execution.kind = N3D_USE_EXEC_UNRESOLVED;
                return execution;
            }

            const int push_slot =
                N3D_RE_FindPushSlotByObject((uint16_t)object_slot);
            if(push_slot < 0)
            {
                execution.kind = N3D_USE_EXEC_UNRESOLVED;
                return execution;
            }

            const n3d_push_direction direction =
                N3D_RE_PushDirectionForOctant(octant);

            const int dest_x =
                (int)execution.target.x +
                (direction.dx > 0 ? 1 : direction.dx < 0 ? -1 : 0);
            const int dest_y =
                (int)execution.target.y +
                (direction.dy > 0 ? 1 : direction.dy < 0 ? -1 : 0);

            if(dest_x < 0 || dest_y < 0 ||
               dest_x >= N3D_MAP_WIDTH || dest_y >= N3D_MAP_HEIGHT)
            {
                execution.kind = N3D_USE_EXEC_PUSH_BLOCKED;
                execution.runtime_slot = push_slot;
                return execution;
            }

            const n3d_map_cell* destination =
                N3D_RE_MapCell((uint8_t)dest_x, (uint8_t)dest_y);

            if(!destination ||
               !N3D_RE_WallPropertyKnown(destination->wall) ||
               !N3D_RE_ObjectPropertyKnown(destination->object))
            {
                execution.kind = N3D_USE_EXEC_UNRESOLVED;
                execution.runtime_slot = push_slot;
                return execution;
            }

            const uint8_t destination_flags =
                (uint8_t)(
                    n3d_wall_property_resolved[destination->wall] |
                    n3d_object_property_resolved[destination->object]);

            if(!N3D_RE_CanStartPush(
                    &n3d_pushes[push_slot],
                    destination_flags))
            {
                execution.kind = N3D_USE_EXEC_PUSH_BLOCKED;
                execution.runtime_slot = push_slot;
                return execution;
            }

            if(!N3D_RE_BeginPush(
                    &n3d_pushes[push_slot],
                    (uint16_t)object_slot,
                    octant))
            {
                execution.kind = N3D_USE_EXEC_PUSH_BLOCKED;
                execution.runtime_slot = push_slot;
                return execution;
            }

            execution.runtime_slot = push_slot;
            execution.kind = N3D_USE_EXEC_PUSH_STARTED;
            return execution;
        }

        case N3D_USE_MAPPED_WALL:
        {
            const uint8_t wall_type = execution.target.mapped_wall_type;

            if(wall_type == 0x03)
            {
                /*
                 * seg4:21D8: ID-card-controlled remote doors/cannons terminal.
                 * Associated OBJECT+01 selects Red/Yellow ID-card bit 0/1.
                 */
                const int object_slot =
                    N3D_RE_FindObjectSlotByCell(
                        execution.target.x,
                        execution.target.y);

                if(object_slot < 0)
                {
                    execution.kind = N3D_USE_EXEC_UNRESOLVED;
                    return execution;
                }

                const uint8_t required_bit =
                    n3d_objects[object_slot].variant;

                if(required_bit >= 2)
                {
                    execution.kind = N3D_USE_EXEC_UNRESOLVED;
                    return execution;
                }

                execution.required_inventory_bit = required_bit;
                execution.runtime_slot = object_slot;
                execution.kind =
                    N3D_RE_HasInventoryBit(
                        n3d_player.id_cards,
                        required_bit)
                        ? N3D_USE_EXEC_REMOTE_TERMINAL_PASSED
                        : N3D_USE_EXEC_REMOTE_TERMINAL_BLOCKED;
                return execution;
            }

            if(wall_type == 0x08)
            {
                execution.kind = N3D_USE_EXEC_SCRIPTED_WALL_REQUEST;
                return execution;
            }

            if(wall_type == 0x09 || wall_type == 0x0A)
            {
                execution.level_delta =
                    (uint8_t)(wall_type == 0x09 ? 1 : 2);
                execution.kind = N3D_USE_EXEC_LEVEL_CHANGE_REQUEST;
                return execution;
            }

            if(wall_type >= 0x0D && wall_type <= 0x14)
            {
                execution.menu_first_wall_type = 0x0D;
                execution.menu_last_wall_type = 0x14;
                execution.menu_variant_index =
                    (uint8_t)(wall_type - 0x0D);
                execution.kind = N3D_USE_EXEC_CLIMB_MENU_REQUEST;
                return execution;
            }

            if(wall_type >= 0x15 && wall_type <= 0x18)
            {
                if(wall_type == 0x15)
                {
                    execution.kind =
                        N3D_RE_HasAllPentagrams()
                            ? N3D_USE_EXEC_PENTAGRAM_GATE_PASSED
                            : N3D_USE_EXEC_PENTAGRAM_GATE_BLOCKED;
                    return execution;
                }

                execution.menu_first_wall_type = 0x15;
                execution.menu_last_wall_type = 0x18;
                execution.menu_variant_index =
                    (uint8_t)(wall_type - 0x15);
                execution.kind = N3D_USE_EXEC_OTHER_SIDE_REQUEST;
                return execution;
            }

            if(wall_type >= 0x19 && wall_type <= 0x1C)
            {
                const uint8_t required_bit =
                    (uint8_t)(wall_type - 0x19);
                execution.required_inventory_bit = required_bit;

                execution.kind =
                    N3D_RE_HasInventoryBit(
                        n3d_player.colored_keys,
                        required_bit)
                        ? N3D_USE_EXEC_KEY_GATE_PASSED
                        : N3D_USE_EXEC_KEY_GATE_BLOCKED;
                return execution;
            }

            if(wall_type >= 0x1D && wall_type <= 0x24)
            {
                execution.menu_first_wall_type = 0x1D;
                execution.menu_last_wall_type = 0x24;
                execution.menu_variant_index =
                    (uint8_t)(wall_type - 0x1D);
                execution.kind = N3D_USE_EXEC_FLOOR_MENU_REQUEST;
                return execution;
            }

            if(wall_type >= 0x25 && wall_type <= 0x2C)
            {
                execution.menu_first_wall_type = 0x25;
                execution.menu_last_wall_type = 0x2C;
                execution.menu_variant_index =
                    (uint8_t)(wall_type - 0x25);
                execution.kind = N3D_USE_EXEC_GO_DOWN_MENU_REQUEST;
                return execution;
            }

            execution.kind = N3D_USE_EXEC_WALL_DEFERRED;
            return execution;
        }

        case N3D_USE_MAPPED_OBJECT:
            execution.kind = N3D_USE_EXEC_OBJECT_DEFERRED;
            return execution;
    }

    execution.kind = N3D_USE_EXEC_UNRESOLVED;
    return execution;
}
