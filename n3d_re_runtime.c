#include "n3d_re_runtime.h"
#include "n3d_re_guard.h"
#include "n3d_re_door.h"
#include "n3d_re_special_runtime.h"
#include "n3d_re_use.h"
#include "n3d_re_definitions.h"
#include "n3d_re_wall_explosion.h"
#include "n3d_re_object_defs.h"
#include "n3d_re_timing.h"
#include <string.h>

n3d_map_cell n3d_map[N3D_MAP_WIDTH * N3D_MAP_HEIGHT];
n3d_object_record n3d_objects[N3D_MAX_OBJECTS];
n3d_object_binding n3d_object_bindings[N3D_MAX_OBJECTS];
n3d_guard_record n3d_guards[N3D_MAX_GUARDS];
n3d_projectile_record n3d_projectiles[N3D_MAX_PROJECTILES];
uint16_t n3d_object_count;
uint16_t n3d_guard_count;

void N3D_RE_ResetRuntime(void)
{
    memset(n3d_map, 0, sizeof(n3d_map));
    memset(n3d_objects, 0, sizeof(n3d_objects));
    memset(n3d_object_bindings, 0, sizeof(n3d_object_bindings));
    memset(n3d_guards, 0, sizeof(n3d_guards));
    memset(n3d_projectiles, 0, sizeof(n3d_projectiles));
    N3D_RE_ClearGuardWakeCache();
    N3D_RE_ResetDoors();
    N3D_RE_ResetPanelsAndPushes();
    N3D_RE_ResetUseLatch();
    N3D_RE_ResetExplodingWalls();
    N3D_RE_ResetObjectDefinitions();
    N3D_RE_ResetGuardLogicClock();
    n3d_object_count = 0;
    n3d_guard_count = 0;
}

int N3D_RE_LoadMapPayload(const uint8_t* payload, size_t payload_size)
{
    if (!payload || payload_size < N3D_MAP_LEVEL_BYTES)
        return 0;

    N3D_RE_ResetRuntime();

    for (int cell = 0; cell < N3D_MAP_WIDTH * N3D_MAP_HEIGHT; ++cell)
    {
        const size_t offset = (size_t)cell * N3D_MAP_CELL_BYTES;
        n3d_map[cell].wall = payload[offset];
        n3d_map[cell].object = payload[offset + 1];

        const uint8_t x = (uint8_t)(cell % N3D_MAP_WIDTH);
        const uint8_t y = (uint8_t)(cell / N3D_MAP_WIDTH);

        if(!N3D_RE_InstantiateMapObject(
                n3d_map[cell].object, x, y, NULL))
            return 0;

        if(N3D_RE_WallPropertyKnown(n3d_map[cell].wall) &&
           (n3d_wall_property_resolved[n3d_map[cell].wall] & 0x08))
        {
            /*
             * Bind the cell to a door slot, but keep its runtime state unknown
             * until the original 14A8..16D5 initialization writes are closed.
             */
            if(N3D_RE_RegisterDoorCell(x, y) < 0)
                return 0;
        }
    }

    return 1;
}

const n3d_map_cell* N3D_RE_MapCell(uint8_t x, uint8_t y)
{
    if (x >= N3D_MAP_WIDTH || y >= N3D_MAP_HEIGHT)
        return NULL;

    return &n3d_map[(size_t)y * N3D_MAP_WIDTH + x];
}

int N3D_RE_FindObjectSlotByCell(uint8_t x, uint8_t y)
{
    for(uint16_t i = 0; i < n3d_object_count; ++i)
    {
        const n3d_object_binding* binding = &n3d_object_bindings[i];
        if(binding->used && binding->cell_x == x && binding->cell_y == y)
            return (int)i;
    }

    return -1;
}

static int N3D_RE_ResolveObjectRuntime(
    uint8_t map_object_id,
    uint8_t* object_class,
    uint8_t* flags)
{
    if(!object_class || !flags || map_object_id == 0)
        return 0;

    if(N3D_RE_ObjectMappingKnown(map_object_id) &&
       N3D_RE_ObjectPropertyKnown(map_object_id))
    {
        *object_class = n3d_object_mapped_type[map_object_id];
        *flags = n3d_object_property_resolved[map_object_id];
        return 1;
    }

    /*
     * Bootstrap fallback for old isolated tests/data without loaded OBJECTS.*
     * definitions. Only the directly recovered Episode-1 guard ranges use it.
     */
    if(N3D_RE_GuardClassFromMapObject(map_object_id, object_class))
    {
        *flags = N3D_RE_ObjectPropertiesForMappedType(*object_class);
        return 1;
    }

    return 0;
}

int N3D_RE_InstantiateMapObject(
    uint8_t map_object_id,
    uint8_t tile_x,
    uint8_t tile_y,
    int* object_slot)
{
    if(object_slot)
        *object_slot = -1;

    uint8_t object_class = 0;
    uint8_t flags = 0;
    if(!N3D_RE_ResolveObjectRuntime(map_object_id, &object_class, &flags))
        return 1; /* unresolved/non-runtime is not a pool failure */

    if(object_class == N3D_PANEL_MAPPED_OBJECT_TYPE)
    {
        return N3D_RE_RegisterPanelCell(tile_x, tile_y) >= 0;
    }

    if((flags & N3D_OBJECT_RUNTIME_PRESENT) == 0)
        return 1;

    if(n3d_object_count >= N3D_MAX_OBJECTS)
        return 0;

    const uint16_t slot = n3d_object_count++;
    n3d_object_record* obj = &n3d_objects[slot];
    memset(obj, 0, sizeof(*obj));

    obj->map_object_id = map_object_id;

    uint8_t definition_id = 0;
    if(N3D_RE_RegisterMapObjectDefinition(
            map_object_id,
            &definition_id))
    {
        obj->sequence_id = definition_id;
    }

    uint8_t variant_index = 0;
    if(N3D_RE_DefinitionVariantIndex(
            &n3d_object_definitions,
            map_object_id,
            &variant_index))
    {
        obj->variant = variant_index;
    }

    obj->flags = flags;
    obj->object_class = object_class;
    obj->world_x =
        (int16_t)(tile_x * N3D_WORLD_UNITS_PER_TILE + N3D_TILE_CENTER_OFFSET);
    obj->world_y =
        (int16_t)(tile_y * N3D_WORLD_UNITS_PER_TILE + N3D_TILE_CENTER_OFFSET);

    n3d_object_bindings[slot].used = 1;
    n3d_object_bindings[slot].cell_x = tile_x;
    n3d_object_bindings[slot].cell_y = tile_y;

    if(flags & N3D_OBJECT_CREATES_GUARD)
    {
        if(n3d_guard_count >= N3D_MAX_GUARDS)
        {
            --n3d_object_count;
            memset(obj, 0, sizeof(*obj));
            memset(&n3d_object_bindings[slot], 0, sizeof(n3d_object_bindings[slot]));
            return 0;
        }

        const uint16_t guard_slot = n3d_guard_count++;
        n3d_guard_record* guard = &n3d_guards[guard_slot];
        memset(guard, 0, sizeof(*guard));

        obj->guard_index = (uint8_t)guard_slot;

        const n3d_guard_initial_profile profile =
            N3D_RE_GuardInitialProfile(object_class);

        guard->object_slot = slot;
        guard->area_id = N3D_GUARD_AREA_UNSET;
        guard->strategy = profile.strategy;
        guard->state = profile.state;
        guard->next_state = profile.next_state;
        guard->transition_flag = profile.perception_mode;
        guard->strength = 0xFF;

        N3D_RE_UpdateGuardAreaForSlot(guard_slot);
    }

    if(object_class == N3D_PUSH_MAPPED_OBJECT_TYPE)
    {
        if(N3D_RE_RegisterPushObject(slot) < 0)
        {
            --n3d_object_count;
            memset(obj, 0, sizeof(*obj));
            memset(&n3d_object_bindings[slot], 0, sizeof(n3d_object_bindings[slot]));
            return 0;
        }
    }

    if(object_slot)
        *object_slot = (int)slot;

    return 1;
}

int N3D_RE_GuardClassFromMapObject(uint8_t id, uint8_t* object_class)
{
    if (!object_class)
        return 0;

    if (id >= 128 && id <= 139) {
        *object_class = (uint8_t)(0x08 + ((id - 128) / 4));
        return 1;
    }

    if (id >= 144 && id <= 159) {
        *object_class = (uint8_t)(0x0B + ((id - 144) / 4));
        return 1;
    }

    if (id >= 160 && id <= 167) {
        *object_class = 0x0F;
        return 1;
    }

    if (id >= 168 && id <= 175) {
        *object_class = 0x10;
        return 1;
    }

    if (id >= 176 && id <= 187) {
        *object_class = (uint8_t)(0x11 + ((id - 176) / 4));
        return 1;
    }

    if (id >= 188 && id <= 195) {
        *object_class = 0x15;
        return 1;
    }

    if (id >= 196 && id <= 203) {
        *object_class = 0x16;
        return 1;
    }

    if (id >= 204 && id <= 207) {
        *object_class = 0x19;
        return 1;
    }

    /* 140 is Dancers/GUARD26 and intentionally stays outside GUARD1..25. */
    return 0;
}

n3d_guard_initial_profile N3D_RE_GuardInitialProfile(uint8_t object_class)
{
    n3d_guard_initial_profile p = {0, N3D_GUARD_STATE_07, N3D_GUARD_STATE_02, 1};

    switch (object_class)
    {
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x11:
        case 0x14:
        case 0x1A:
            p.perception_mode = 0;
            break;

        case 0x12:
        case 0x13:
            p.strategy = 3;
            p.perception_mode = 0;
            break;

        case 0x15:
        case 0x16:
            p.next_state = 0;
            break;

        case 0x19:
            p.strategy = 4;
            p.state = N3D_GUARD_STATE_0E;
            break;

        case 0x21:
            p.state = 0;
            p.next_state = 0;
            break;

        default:
            break;
    }

    return p;
}

int N3D_RE_RegisterGuardFromMap(uint8_t map_object_id, uint8_t tile_x, uint8_t tile_y)
{
    uint8_t object_class = 0;
    uint8_t flags = 0;

    if(!N3D_RE_ResolveObjectRuntime(map_object_id, &object_class, &flags) ||
       (flags & N3D_OBJECT_CREATES_GUARD) == 0)
        return 0;

    const uint16_t old_object_count = n3d_object_count;
    const uint16_t old_guard_count = n3d_guard_count;

    if(!N3D_RE_InstantiateMapObject(map_object_id, tile_x, tile_y, NULL))
        return 0;

    return n3d_object_count == old_object_count + 1 &&
           n3d_guard_count == old_guard_count + 1;
}
