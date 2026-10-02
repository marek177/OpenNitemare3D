#include "../n3d_re_runtime.h"
#include "../n3d_re_guard.h"

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

    uint8_t payload[N3D_MAP_LEVEL_BYTES] = {0};
    const int bat_cell = 2 * N3D_MAP_WIDTH + 1;
    payload[bat_cell * N3D_MAP_CELL_BYTES] = 7;
    payload[bat_cell * N3D_MAP_CELL_BYTES + 1] = 128;

    const int cannon_cell = 6 * N3D_MAP_WIDTH + 5;
    payload[cannon_cell * N3D_MAP_CELL_BYTES] = 9;
    payload[cannon_cell * N3D_MAP_CELL_BYTES + 1] = 204;

    assert(N3D_RE_LoadMapPayload(payload, sizeof(payload)));
    assert(N3D_RE_MapCell(1, 2)->wall == 7);
    assert(N3D_RE_MapCell(1, 2)->object == 128);
    assert(N3D_RE_MapCell(5, 6)->wall == 9);
    assert(N3D_RE_MapCell(5, 6)->object == 204);
    assert(N3D_RE_MapCell(64, 0) == NULL);
    assert(n3d_object_count == 2);
    assert(n3d_guard_count == 2);
    assert(n3d_objects[0].object_class == 0x08);
    assert(n3d_objects[0].world_x == 96);
    assert(n3d_objects[0].world_y == 160);
    assert(n3d_objects[1].object_class == 0x19);
    assert(n3d_guards[1].strategy == 4);
    assert(n3d_guards[1].state == N3D_GUARD_STATE_0E);

    N3D_RE_ResetRuntime();

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

    n3d_guard_move_vector step = N3D_RE_GuardDirectionalStep(1, 0);
    assert(step.dx == 8 && step.dy == 0);
    step = N3D_RE_GuardDirectionalStep(1, 2);
    assert(step.dx == 16 && step.dy == 0);
    step = N3D_RE_GuardDirectionalStep(7, 0);
    assert(step.dx == 0 && step.dy == -8);

    assert(N3D_RE_State13InitialTimer(0) == 8);
    assert(N3D_RE_State13InitialTimer(79) == 87);
    assert(N3D_RE_State13InitialTimer(80) == 8);

    n3d_state13_step_result s13 = N3D_RE_StepState13(9, 1);
    assert(s13.next_timer == 8);
    assert(s13.play_movement_sound);
    assert(!s13.attempt_movement);

    uint16_t timer = 8;
    int move_attempts = 0;
    for(int i = 0; i < 8; ++i)
    {
        s13 = N3D_RE_StepState13(timer, 0);
        assert(s13.attempt_movement);
        assert(!s13.commit_movement);
        timer = s13.next_timer;
        ++move_attempts;
    }
    assert(move_attempts == 8 && timer == 0);
    s13 = N3D_RE_StepState13(0, 1);
    assert(s13.clear_strategy_and_enter_state2);

    uint8_t selector = 0;
    assert(N3D_RE_DoorSelectorFromWallId(0x70, &selector) && selector == 0);
    assert(N3D_RE_DoorSelectorFromWallId(0x77, &selector) && selector == 7);
    assert(!N3D_RE_DoorSelectorFromWallId(0x78, &selector));
    assert(N3D_RE_DoorSelectorFromWallId(0xAD, &selector) && selector == 61);
    assert(N3D_RE_DoorSelectorFromWallId(0xAE, &selector) && selector == 62);

    N3D_RE_ResetRuntime();
    assert(N3D_RE_RegisterGuardFromMap(128, 1, 1));
    assert(N3D_RE_RegisterGuardFromMap(132, 2, 2));
    n3d_guards[0].definition_id = 5;
    n3d_guards[1].definition_id = 5;
    n3d_guards[0].state = N3D_GUARD_STATE_07;
    n3d_guards[1].state = N3D_GUARD_STATE_08;
    n3d_guards[0].strategy = 0;
    n3d_guards[1].strategy = 0;

    uint32_t rng_state = 1;
    int woke = N3D_RE_WakeGuards(5, &rng_state);
    assert(woke == 2);
    assert(n3d_guards[0].state == N3D_GUARD_STATE_01);
    assert(n3d_guards[1].state == N3D_GUARD_STATE_01);
    assert(n3d_guards[0].timer >= 0 && n3d_guards[0].timer <= 7);
    assert(n3d_guards[1].timer >= 0 && n3d_guards[1].timer <= 7);
    assert(N3D_RE_WakeGuards(5, &rng_state) == 0);

    puts("C-rewrite recovered runtime self-test: PASS");
    return 0;
}
