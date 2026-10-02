#include "../n3d_re_runtime.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    uint8_t object_class = 0;

    assert(N3D_RE_GuardClassFromMapObject(128, &object_class) && object_class == 0x08);
    assert(N3D_RE_GuardClassFromMapObject(139, &object_class) && object_class == 0x0A);
    assert(N3D_RE_GuardClassFromMapObject(144, &object_class) && object_class == 0x0B);
    assert(N3D_RE_GuardClassFromMapObject(160, &object_class) && object_class == 0x0F);
    assert(N3D_RE_GuardClassFromMapObject(168, &object_class) && object_class == 0x10);
    assert(N3D_RE_GuardClassFromMapObject(176, &object_class) && object_class == 0x11);
    assert(N3D_RE_GuardClassFromMapObject(180, &object_class) && object_class == 0x12);
    assert(N3D_RE_GuardClassFromMapObject(184, &object_class) && object_class == 0x13);
    assert(N3D_RE_GuardClassFromMapObject(188, &object_class) && object_class == 0x15);
    assert(N3D_RE_GuardClassFromMapObject(196, &object_class) && object_class == 0x16);
    assert(N3D_RE_GuardClassFromMapObject(204, &object_class) && object_class == 0x19);
    assert(!N3D_RE_GuardClassFromMapObject(140, &object_class));

    N3D_RE_ResetRuntime();
    assert(n3d_object_count == 0);
    assert(n3d_guard_count == 0);

    assert(N3D_RE_RegisterGuardFromMap(128, 1, 2));
    assert(n3d_object_count == 1);
    assert(n3d_guard_count == 1);
    assert(n3d_objects[0].map_object_id == 128);
    assert(n3d_objects[0].object_class == 0x08);
    assert(n3d_objects[0].guard_index == 0);
    assert(n3d_objects[0].world_x == 96);
    assert(n3d_objects[0].world_y == 160);
    assert(n3d_guards[0].object_slot == 0);
    assert(n3d_guards[0].strength == 0xFF);
    assert(n3d_guards[0].strategy == 0);
    assert(n3d_guards[0].state == N3D_GUARD_STATE_07);
    assert(n3d_guards[0].next_state == N3D_GUARD_STATE_02);

    assert(N3D_RE_RegisterGuardFromMap(180, 3, 4));
    assert(n3d_objects[1].object_class == 0x12);
    assert(n3d_guards[1].strategy == 3);
    assert(n3d_guards[1].state == N3D_GUARD_STATE_07);

    assert(N3D_RE_RegisterGuardFromMap(204, 5, 6));
    assert(n3d_objects[2].object_class == 0x19);
    assert(n3d_guards[2].strategy == 4);
    assert(n3d_guards[2].state == N3D_GUARD_STATE_0E);

    assert(!N3D_RE_RegisterGuardFromMap(140, 7, 8));
    assert(n3d_object_count == 3);
    assert(n3d_guard_count == 3);

    puts("C-rewrite recovered runtime self-test: PASS");
    return 0;
}
