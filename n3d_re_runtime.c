#include "n3d_re_runtime.h"
#include <string.h>

n3d_object_record n3d_objects[N3D_MAX_OBJECTS];
n3d_guard_record n3d_guards[N3D_MAX_GUARDS];
n3d_projectile_record n3d_projectiles[N3D_MAX_PROJECTILES];
uint16_t n3d_object_count;
uint16_t n3d_guard_count;

void N3D_RE_ResetRuntime(void)
{
    memset(n3d_objects, 0, sizeof(n3d_objects));
    memset(n3d_guards, 0, sizeof(n3d_guards));
    memset(n3d_projectiles, 0, sizeof(n3d_projectiles));
    n3d_object_count = 0;
    n3d_guard_count = 0;
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
    uint8_t object_class;
    if (!N3D_RE_GuardClassFromMapObject(map_object_id, &object_class))
        return 0;

    if (n3d_object_count >= N3D_MAX_OBJECTS || n3d_guard_count >= N3D_MAX_GUARDS)
        return 0;

    const uint16_t object_slot = n3d_object_count++;
    const uint16_t guard_slot = n3d_guard_count++;

    n3d_object_record* obj = &n3d_objects[object_slot];
    n3d_guard_record* guard = &n3d_guards[guard_slot];

    memset(obj, 0, sizeof(*obj));
    memset(guard, 0, sizeof(*guard));

    obj->map_object_id = map_object_id;
    obj->flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_CREATES_GUARD;
    obj->object_class = object_class;
    obj->guard_index = (uint8_t)guard_slot;
    obj->world_x = (int16_t)(tile_x * N3D_WORLD_UNITS_PER_TILE + N3D_TILE_CENTER_OFFSET);
    obj->world_y = (int16_t)(tile_y * N3D_WORLD_UNITS_PER_TILE + N3D_TILE_CENTER_OFFSET);

    n3d_guard_initial_profile profile = N3D_RE_GuardInitialProfile(object_class);
    guard->object_slot = object_slot;
    guard->strategy = profile.strategy;
    guard->state = profile.state;
    guard->next_state = profile.next_state;
    guard->strength = 0xFF;

    /*
     * perception_mode is recovered by class, but its exact destination field
     * remains partial. Do not write it into an unresolved GUARD byte yet.
     */
    return 1;
}
