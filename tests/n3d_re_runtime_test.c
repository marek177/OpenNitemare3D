#include "../n3d_re_runtime.h"
#include "../n3d_re_guard.h"
#include "../n3d_re_collision.h"
#include "../n3d_re_player.h"
#include "../n3d_re_projectile.h"
#include "../n3d_re_combat.h"
#include "../n3d_re_definitions.h"
#include "../n3d_re_door.h"
#include "../n3d_re_special_runtime.h"
#include "../n3d_re_use.h"
#include "../n3d_re_pickup.h"
#include "../n3d_re_trig.h"
#include "../n3d_re_movement.h"
#include "../n3d_re_timing.h"
#include "../n3d_re_controls.h"
#include "../n3d_re_weapon.h"
#include "../n3d_re_save.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int test_door_passable(uint8_t x, uint8_t y, void* user)
{
    (void)x;
    (void)y;
    return *(int*)user;
}

static void test_touch(uint8_t x, uint8_t y, void* user)
{
    (void)x;
    (void)y;
    ++*(int*)user;
}

static void write_s16_le(uint8_t* bytes, int offset, int16_t value)
{
    const uint16_t raw = (uint16_t)value;
    bytes[offset] = (uint8_t)(raw & 0xFF);
    bytes[offset + 1] = (uint8_t)(raw >> 8);
}

int main(void)
{
    uint8_t trig_fixture[N3D_TRIG_FILE_BYTES] = {0};
    write_s16_le(trig_fixture, 45 * 2, 724);
    write_s16_le(trig_fixture, 90 * 2, 1024);
    write_s16_le(trig_fixture, 180 * 2, 0);
    write_s16_le(trig_fixture, 270 * 2, -1024);
    write_s16_le(
        trig_fixture,
        N3D_TRIG_ANGLE_COUNT * 2 + 0 * 2,
        1024);
    write_s16_le(
        trig_fixture,
        N3D_TRIG_ANGLE_COUNT * 2 + 45 * 2,
        724);
    write_s16_le(
        trig_fixture,
        N3D_TRIG_ANGLE_COUNT * 2 + 180 * 2,
        -1024);
    write_s16_le(
        trig_fixture,
        N3D_TRIG_ANGLE_COUNT * 2 + 270 * 2,
        0);

    FILE* trig_file = fopen("N3D_TRIG_Q10_TEST.BIN", "wb");
    assert(trig_file != NULL);
    assert(fwrite(
        trig_fixture, 1, sizeof(trig_fixture), trig_file) ==
        sizeof(trig_fixture));
    fclose(trig_file);

    assert(N3D_RE_LoadTrigQ10("N3D_TRIG_Q10_TEST.BIN"));
    assert(n3d_trig.loaded);
    assert(N3D_RE_NormalizeAngle(-1) == 359);
    assert(N3D_RE_NormalizeAngle(360) == 0);
    assert(N3D_RE_SinQ10(45) == 724);
    assert(N3D_RE_SinQ10(90) == 1024);
    assert(N3D_RE_CosQ10(0) == 1024);
    assert(N3D_RE_CosQ10(45) == 724);
    assert(N3D_RE_CosQ10(90) == 0);
    assert(N3D_RE_CoarseOctant(44) == 0);
    assert(N3D_RE_CoarseOctant(45) == 1);
    assert(N3D_RE_RoundedOctant(22) == 0);
    assert(N3D_RE_RoundedOctant(23) == 1);
    assert(N3D_RE_RoundedOctant(359) == 0);

    n3d_line_state exact_line = {0};

    assert(N3D_RE_InitLineStateFromAngle(0, &exact_line));
    assert(exact_line.sin_q10 == 0);
    assert(exact_line.cos_q10 == 1024);
    assert(exact_line.axis_flag == 0);
    assert(exact_line.error == -1024);
    assert(exact_line.twice_minor == 0);
    assert(exact_line.twice_minor_minus_major == -2048);
    assert(exact_line.step_x == 1);
    assert(exact_line.step_y == -1);

    assert(N3D_RE_InitLineStateFromAngle(45, &exact_line));
    assert(exact_line.sin_q10 == 724);
    assert(exact_line.cos_q10 == 724);
    assert(exact_line.axis_flag == 0);
    assert(exact_line.error == 724);
    assert(exact_line.twice_minor == 1448);
    assert(exact_line.twice_minor_minus_major == 0);
    assert(exact_line.step_x == 1);
    assert(exact_line.step_y == -1);

    assert(N3D_RE_InitLineStateFromAngle(90, &exact_line));
    assert(exact_line.sin_q10 == 1024);
    assert(exact_line.cos_q10 == 0);
    assert(exact_line.axis_flag == 1);
    assert(exact_line.error == -1024);
    assert(exact_line.twice_minor == 0);
    assert(exact_line.twice_minor_minus_major == -2048);
    assert(exact_line.step_x == 1);

    assert(N3D_RE_InitLineStateFromAngle(180, &exact_line));
    assert(exact_line.axis_flag == 0);
    assert(exact_line.step_y == 1);

    assert(N3D_RE_InitLineStateFromAngle(270, &exact_line));
    assert(exact_line.axis_flag == 1);
    assert(exact_line.step_x == -1);

    remove("N3D_TRIG_Q10_TEST.BIN");

    n3d_timing_parameters timing =
        N3D_RE_ComputeTimingParameters(10);
    assert(timing.raw_mean_ms == 10);
    assert(timing.effective_ms == 40);
    assert(timing.slow_updates_per_second == 25);
    assert(timing.movement_substeps == 10);
    assert(timing.turn_degrees == 5);
    assert(timing.projectile_substeps == 20);

    timing = N3D_RE_ComputeTimingParameters(40);
    assert(timing.effective_ms == 40);
    assert(timing.movement_substeps == 10);
    assert(timing.turn_degrees == 5);
    assert(timing.projectile_substeps == 20);

    timing = N3D_RE_ComputeTimingParameters(50);
    assert(timing.effective_ms == 50);
    assert(timing.slow_updates_per_second == 20);
    assert(timing.movement_substeps == 13);
    assert(timing.turn_degrees == 6);
    assert(timing.projectile_substeps == 26);

    N3D_RE_ResetTimingCalibration();
    assert(!N3D_RE_SampleTimingFrame(0));
    assert(n3d_timing_calibration_state.sample_count == 0);

    for(int i = 0; i < 4; ++i)
    {
        assert(!N3D_RE_SampleTimingFrame(10));
        assert(!n3d_timing_calibration_state.complete);
    }
    assert(N3D_RE_SampleTimingFrame(10));
    assert(n3d_timing_calibration_state.complete);
    assert(n3d_timing_calibration_state.sample_count == 5);
    assert(n3d_timing.raw_mean_ms == 10);
    assert(n3d_timing.effective_ms == 40);
    assert(n3d_timing.movement_substeps == 10);
    assert(n3d_timing.turn_degrees == 5);
    assert(n3d_timing.projectile_substeps == 20);
    assert(!N3D_RE_SampleTimingFrame(100));

    N3D_RE_ResetTimingCalibration();
    assert(!N3D_RE_SampleTimingFrame(40));
    assert(!N3D_RE_SampleTimingFrame(40));
    assert(!N3D_RE_SampleTimingFrame(50));
    assert(!N3D_RE_SampleTimingFrame(50));
    assert(N3D_RE_SampleTimingFrame(60));
    /* Integer elapsed/5: (40+40+50+50+60)/5 == 48. */
    assert(n3d_timing.raw_mean_ms == 48);
    assert(n3d_timing.effective_ms == 48);
    assert(n3d_timing.movement_substeps == 12);
    assert(n3d_timing.turn_degrees == 6);
    assert(n3d_timing.projectile_substeps == 24);

    N3D_RE_SetTimingFromRawMean(40);
    assert(n3d_timing.movement_substeps == 10);
    assert(n3d_timing.turn_degrees == 5);

    n3d_control_steps control_steps =
        N3D_RE_ControlStepsForInput(0);
    assert(control_steps.movement_substeps == 10);
    assert(control_steps.turn_degrees == 5);

    control_steps =
        N3D_RE_ControlStepsForInput(N3D_INPUT_FAST);
    assert(control_steps.movement_substeps == 20);
    assert(control_steps.turn_degrees == 10);

    control_steps =
        N3D_RE_ControlStepsForInput(N3D_INPUT_FINE);
    assert(control_steps.movement_substeps == 1);
    assert(control_steps.turn_degrees == 1);

    control_steps =
        N3D_RE_ControlStepsForInput(
            N3D_INPUT_FAST | N3D_INPUT_FINE);
    assert(control_steps.movement_substeps == 1);
    assert(control_steps.turn_degrees == 1);

    assert(N3D_RE_ForwardAngle(350) == 350);
    assert(N3D_RE_BackwardAngle(350) == 170);
    assert(N3D_RE_StrafeLeftAngle(10) == 280);
    assert(N3D_RE_StrafeRightAngle(350) == 80);
    assert(N3D_RE_LeftTurnDelta(0) == -5);
    assert(N3D_RE_RightTurnDelta(0) == 5);
    assert(N3D_RE_LeftTurnDelta(N3D_INPUT_FAST) == -10);
    assert(N3D_RE_RightTurnDelta(N3D_INPUT_FAST) == 10);
    assert(N3D_RE_LeftTurnDelta(N3D_INPUT_FINE) == -1);
    assert(N3D_RE_RightTurnDelta(N3D_INPUT_FINE) == 1);
    assert(N3D_RE_LeftTurnDelta(N3D_INPUT_STRAFE) == 0);
    assert(N3D_RE_RightTurnDelta(N3D_INPUT_STRAFE) == 0);

    N3D_RE_ResetPlayer();
    assert(n3d_player.angle_degrees == 0);
    assert(n3d_player.coarse_octant == 0);
    assert(n3d_player.rounded_octant == 0);
    assert(n3d_player.direction_mask_99 == 0x01);

    N3D_RE_SetPlayerAngle(44);
    assert(n3d_player.angle_degrees == 44);
    assert(n3d_player.coarse_octant == 0);
    assert(n3d_player.rounded_octant == 1);
    assert(n3d_player.direction_mask_99 == 0x01);

    N3D_RE_SetPlayerAngle(135);
    assert(n3d_player.coarse_octant == 3);
    assert(n3d_player.rounded_octant == 3);
    assert(n3d_player.direction_mask_99 == 0x08);

    N3D_RE_TurnPlayer(-140);
    assert(n3d_player.angle_degrees == 355);
    assert(n3d_player.coarse_octant == 7);
    assert(n3d_player.rounded_octant == 0);
    assert(n3d_player.direction_mask_99 == 0x80);

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

    /*
     * Guard wake cache is AREA-based, not DOOR-based. Class-0x44 variants
     * use rawWallId-firstRawId semantics and persist off AREA marker cells.
     */
    n3d_wall_mapping_known[10] = 1;
    n3d_wall_property_known[10] = 1;
    n3d_wall_mapped_type[10] = 0x44;
    n3d_wall_property_resolved[10] =
        N3D_RE_WallPropertiesForMappedType(0x44);

    n3d_wall_mapping_known[11] = 1;
    n3d_wall_property_known[11] = 1;
    n3d_wall_mapped_type[11] = 0x44;
    n3d_wall_property_resolved[11] =
        N3D_RE_WallPropertiesForMappedType(0x44);

    n3d_wall_mapping_known[15] = 1;
    n3d_wall_property_known[15] = 1;
    n3d_wall_mapped_type[15] = 0x44;
    n3d_wall_property_resolved[15] =
        N3D_RE_WallPropertiesForMappedType(0x44);

    uint8_t area_id = 0xFF;
    assert(N3D_RE_AreaIdFromWallId(10, &area_id) && area_id == 0);
    assert(N3D_RE_AreaIdFromWallId(11, &area_id) && area_id == 1);
    assert(N3D_RE_AreaIdFromWallId(15, &area_id) && area_id == 5);
    assert(!N3D_RE_AreaIdFromWallId(9, &area_id));

    /* Player DAT_4C1C initializes to zero and persists off AREA markers. */
    N3D_RE_ResetRuntime();
    n3d_map[1 * N3D_MAP_WIDTH + 1].wall = 15;
    N3D_RE_InitPlayerAtTile(1, 1);
    assert(n3d_player.area_id == 5);

    n3d_map[1 * N3D_MAP_WIDTH + 2].wall = 0;
    uint8_t area_event = 0;
    assert(N3D_RE_CommitPlayerWorldPosition(
        2 * N3D_WORLD_UNITS_PER_TILE + N3D_TILE_CENTER_OFFSET,
        1 * N3D_WORLD_UNITS_PER_TILE + N3D_TILE_CENTER_OFFSET,
        &area_event));
    assert(n3d_player.area_id == 5);

    /* GUARD+0x0E initializes FF, adopts AREA id on class-44 cells, and persists. */
    N3D_RE_ResetRuntime();
    n3d_map[1 * N3D_MAP_WIDTH + 1].wall = 15;
    n3d_map[2 * N3D_MAP_WIDTH + 2].wall = 15;
    n3d_map[3 * N3D_MAP_WIDTH + 3].wall = 15;

    assert(N3D_RE_RegisterGuardFromMap(128, 1, 1));
    assert(N3D_RE_RegisterGuardFromMap(132, 2, 2));
    assert(N3D_RE_RegisterGuardFromMap(136, 3, 3));
    assert(n3d_guards[0].area_id == 5);
    assert(n3d_guards[1].area_id == 5);
    assert(n3d_guards[2].area_id == 5);

    n3d_guards[0].state = N3D_GUARD_STATE_07;
    n3d_guards[1].state = N3D_GUARD_STATE_08;
    n3d_guards[2].state = N3D_GUARD_STATE_07;
    n3d_guards[0].strategy = 0;
    n3d_guards[1].strategy = 0;
    n3d_guards[2].strategy = 1;

    uint32_t rng_state = 1;
    assert(N3D_RE_WakeGuardsWithRng(0, &rng_state) == 0);
    assert(n3d_guard_wake_cache[0] == 0);

    int woke = N3D_RE_WakeGuardsWithRng(5, &rng_state);
    assert(woke == 2);
    assert(n3d_guards[0].state == N3D_GUARD_STATE_01);
    assert(n3d_guards[1].state == N3D_GUARD_STATE_01);
    assert(n3d_guards[0].timer == 1); /* CRT rand seed1: 41 % 8 */
    assert(n3d_guards[1].timer == 3); /* next: 18467 % 8 */
    assert(n3d_guards[2].state == N3D_GUARD_STATE_07);
    assert(N3D_RE_WakeGuardsWithRng(5, &rng_state) == 0);

    n3d_objects[n3d_guards[0].object_slot].world_x =
        4 * N3D_WORLD_UNITS_PER_TILE + N3D_TILE_CENTER_OFFSET;
    n3d_objects[n3d_guards[0].object_slot].world_y =
        4 * N3D_WORLD_UNITS_PER_TILE + N3D_TILE_CENTER_OFFSET;

    n3d_map[4 * N3D_MAP_WIDTH + 4].wall = 0;
    assert(!N3D_RE_UpdateGuardAreaForSlot(0));
    assert(n3d_guards[0].area_id == 5);

    n3d_map[4 * N3D_MAP_WIDTH + 4].wall = 11;
    assert(N3D_RE_UpdateGuardAreaForSlot(0));
    assert(n3d_guards[0].area_id == 1);

    /*
     * Successful FIRE uses the player's persistent AREA id and shared original
     * CRT RNG. Hitscan avoids a trig-table dependency in this isolated test.
     */
    N3D_RE_ClearGuardWakeCache();
    N3D_RE_ResetOriginalRng();

    n3d_guards[0].area_id = 5;
    n3d_guards[1].area_id = 5;
    n3d_guards[0].state = N3D_GUARD_STATE_07;
    n3d_guards[1].state = N3D_GUARD_STATE_08;
    n3d_guards[0].strategy = 0;
    n3d_guards[1].strategy = 0;

    N3D_RE_ResetPlayer();
    n3d_player.area_id = 5;
    n3d_player.active_weapon = N3D_WEAPON_SILVER_PISTOL;
    n3d_player.silver_ammo = 5;

    n3d_fire_result area_fire =
        N3D_RE_TryBeginPlayerFire(20);
    assert(area_fire.kind == N3D_FIRE_HITSCAN_READY);
    assert(area_fire.guards_woken == 2);
    assert(n3d_player.silver_ammo == 4);
    assert(n3d_guards[0].state == N3D_GUARD_STATE_01);
    assert(n3d_guards[1].state == N3D_GUARD_STATE_01);
    assert(n3d_guards[0].timer == 1);
    assert(n3d_guards[1].timer == 3);

    for(unsigned i = 0; i < 256; ++i)
    {
        uint8_t expected_wall = 0;
        if(i >= 0x01 && i <= 0x30) expected_wall |= 0x04;
        if(i >= 0x2E && i <= 0x2F) expected_wall |= 0x10;
        if(i >= 0x31 && i <= 0x40) expected_wall |= 0x08;
        if(expected_wall & (0x04 | 0x08)) expected_wall |= 0x01;
        if(i >= 0x01 && i <= 0x40) expected_wall |= 0x02;
        if(i >= 0x47 && i <= 0x48) expected_wall |= 0x40;
        assert(N3D_RE_WallPropertiesForMappedType((uint8_t)i) == expected_wall);

        uint8_t expected_object = 0;
        if(i >= 0x06 && i <= 0x3D) expected_object |= 0x01;
        if(i >= 0x08 && i <= 0x2D) expected_object |= 0x02;
        if(i >= 0x2F && i <= 0x3D) expected_object |= 0x04;
        if(i >= 0x08 && i <= 0x25) expected_object |= 0x08;
        if(i == 0x2A) expected_object |= 0x20;
        if(i == 0x04) expected_object |= 0x40;
        assert(N3D_RE_ObjectPropertiesForMappedType((uint8_t)i) == expected_object);
    }

    assert(N3D_RE_DoorStateAllowsPassage(0));
    assert(N3D_RE_DoorStateAllowsPassage(4));
    assert(!N3D_RE_DoorStateAllowsPassage(1));
    assert(!N3D_RE_DoorStateAllowsPassage(256));

    assert(N3D_RE_WorldToTile(-65) == -2);
    assert(N3D_RE_WorldToTile(-64) == -1);
    assert(N3D_RE_WorldToTile(-1) == -1);
    assert(N3D_RE_WorldToTile(0) == 0);
    assert(N3D_RE_WorldToTile(63) == 0);
    assert(N3D_RE_WorldToTile(64) == 1);

    uint16_t map_offset = 0;
    assert(N3D_RE_MapCellByteOffset(0, 0, &map_offset) && map_offset == 0);
    assert(N3D_RE_MapCellByteOffset(63, 63, &map_offset) && map_offset == 8190);
    assert(!N3D_RE_MapCellByteOffset(-1, 0, &map_offset));
    assert(!N3D_RE_MapCellByteOffset(64, 0, &map_offset));

    n3d_post_move_result post = N3D_RE_PostMoveCell(64, 128, 0, 2);
    assert(post.valid);
    assert(post.tile_x == 1 && post.tile_y == 2);
    assert(post.map_byte_offset == (uint16_t)((2 * 64 + 1) * 2));
    assert(post.entered_tile_event == N3D_ENTERED_TILE_EVENT);

    uint8_t mapped_walls[256] = {0};
    uint8_t mapped_objects[256] = {0};
    n3d_byte_table wall_props = {{0}};
    n3d_byte_table object_props = {{0}};

    mapped_walls[10] = 0x01;
    mapped_objects[20] = 0x08;
    N3D_RE_BuildWallProperties(mapped_walls, &wall_props);
    N3D_RE_BuildObjectProperties(mapped_objects, &object_props);

    N3D_RE_ResetRuntime();
    n3d_map[0].wall = 10;
    n3d_map[1].wall = 0;
    assert(N3D_RE_TestLeadingEdgePair(
               0, 0, 1, 0, 1,
               &wall_props, &object_props, NULL) == 0);

    mapped_walls[10] = 0x31;
    N3D_RE_BuildWallProperties(mapped_walls, &wall_props);
    int door_open = 0;
    n3d_collision_callbacks callbacks = {
        test_door_passable,
        NULL,
        NULL,
        &door_open
    };
    assert(N3D_RE_TestLeadingEdgePair(
               0, 0, 1, 0, 1,
               &wall_props, &object_props, &callbacks) == 0);
    door_open = 1;
    assert(N3D_RE_TestLeadingEdgePair(
               0, 0, 1, 0, 1,
               &wall_props, &object_props, &callbacks) == 1);

    mapped_walls[10] = 0;
    mapped_objects[20] = 0x08;
    N3D_RE_BuildWallProperties(mapped_walls, &wall_props);
    N3D_RE_BuildObjectProperties(mapped_objects, &object_props);
    n3d_map[0].wall = 10;
    n3d_map[0].object = 20;
    assert(N3D_RE_TestLeadingEdgePair(
               0, 0, 1, 0, 1,
               &wall_props, &object_props, NULL) == 0);

    mapped_objects[20] = 0x2F;
    N3D_RE_BuildObjectProperties(mapped_objects, &object_props);
    int touch_count = 0;
    callbacks.door_passable = NULL;
    callbacks.object_touch = test_touch;
    callbacks.user = &touch_count;
    assert(N3D_RE_TestLeadingEdgePair(
               0, 0, 1, 0, 1,
               &wall_props, &object_props, &callbacks) == 1);
    assert(touch_count == 1);

    n3d_guard_record state_guard = {0};
    state_guard.state = N3D_GUARD_STATE_01;
    state_guard.timer = 2;
    assert(N3D_RE_TickState01(&state_guard));
    assert(state_guard.state == N3D_GUARD_STATE_01 && state_guard.timer == 1);
    assert(N3D_RE_TickState01(&state_guard));
    assert(state_guard.state == N3D_GUARD_STATE_02 && state_guard.timer == 0);

    state_guard.state = N3D_GUARD_STATE_00;
    state_guard.next_state = N3D_GUARD_STATE_07;
    assert(N3D_RE_CompleteDeferredState(&state_guard));
    assert(state_guard.state == N3D_GUARD_STATE_07);

    state_guard.state = N3D_GUARD_STATE_12;
    state_guard.next_state = N3D_GUARD_STATE_02;
    assert(N3D_RE_CompleteDeferredState(&state_guard));
    assert(state_guard.state == N3D_GUARD_STATE_02);

    state_guard.state = N3D_GUARD_STATE_06;
    assert(N3D_RE_CompleteState06(&state_guard));
    assert(state_guard.state == N3D_GUARD_STATE_03);

    state_guard.state = N3D_GUARD_STATE_11;
    state_guard.strategy = 2;
    assert(N3D_RE_CompleteState11(&state_guard));
    assert(state_guard.strategy == 0 && state_guard.state == N3D_GUARD_STATE_07);

    state_guard.state = N3D_GUARD_STATE_PAIN;
    state_guard.next_state = N3D_GUARD_STATE_07;
    assert(N3D_RE_CompletePainState15(&state_guard));
    assert(state_guard.state == N3D_GUARD_STATE_07);

    state_guard.state = N3D_GUARD_STATE_07;
    state_guard.strategy = 0;
    assert(N3D_RE_ResolveState07Perception(&state_guard, 1, 1, 0));
    assert(state_guard.state == N3D_GUARD_STATE_07);
    assert(N3D_RE_ResolveState07Perception(&state_guard, 0, 1, 0));
    assert(state_guard.state == N3D_GUARD_STATE_02);

    state_guard.state = N3D_GUARD_STATE_07;
    state_guard.strategy = 3;
    state_guard.octant = 3;
    assert(N3D_RE_ResolveState07Perception(&state_guard, 0, 1, 1));
    assert(state_guard.state == N3D_GUARD_STATE_TIMED_DIRECTIONAL_MOVE);
    assert(state_guard.timer == 9);
    assert(state_guard.move_x == 0 && state_guard.move_y == 8);

    N3D_RE_InitPlayerAtTile(10, 20);
    assert(n3d_player.world_x == 10 * 64 + 32);
    assert(n3d_player.world_y == 20 * 64 + 32);
    assert(n3d_player.tile_x == 10 && n3d_player.tile_y == 20);
    assert(n3d_player.map_cell_offset == (uint16_t)((20 * 64 + 10) * 2));
    assert(n3d_player.health == 100);
    assert(n3d_player.active_weapon == N3D_WEAPON_NONE);

    n3d_player.health = 99;
    assert(N3D_RE_ApplyFixedHealthPickup(20));
    assert(n3d_player.health == 119);
    assert(N3D_RE_ClampPlayerHealthForHud() == 100);
    assert(n3d_player.health == 100);
    assert(!N3D_RE_ApplyFixedHealthPickup(20));

    n3d_player.health = 50;
    n3d_player.game_state = 1;
    n3d_player.omnipotent = 0;
    assert(N3D_RE_ApplyEnemyDamage(10) == N3D_PLAYER_DAMAGE_NONLETHAL);
    assert(n3d_player.health == 40);
    assert(N3D_RE_ApplyEnemyDamage(40) == N3D_PLAYER_DAMAGE_LETHAL);
    assert(n3d_player.health == 0 && n3d_player.game_state == 2);

    n3d_player.health = 50;
    n3d_player.game_state = 1;
    n3d_player.omnipotent = 1;
    assert(N3D_RE_ApplyEnemyDamage(50) == N3D_PLAYER_DAMAGE_SUPPRESSED_OMNIPOTENT);
    assert(n3d_player.health == 50);

    uint8_t mask = 0;
    N3D_RE_GrantInventoryBit(&mask, 3);
    assert(mask == 0x08 && N3D_RE_HasInventoryBit(mask, 3));
    assert(!N3D_RE_HasInventoryBit(mask, 8));

    n3d_player.pentagrams = 0x07;
    assert(!N3D_RE_HasAllPentagrams());
    n3d_player.pentagrams = 0x0F;
    assert(N3D_RE_HasAllPentagrams());

    n3d_player.input_mask = N3D_INPUT_FORWARD | N3D_INPUT_FIRE | N3D_INPUT_USE;
    assert(N3D_RE_HasInput(N3D_INPUT_FORWARD));
    assert(N3D_RE_HasInput(N3D_INPUT_FIRE));
    assert(N3D_RE_HasInput(N3D_INPUT_USE));
    assert(!N3D_RE_HasInput(N3D_INPUT_BACKWARD));

    N3D_RE_InitPlayerAtTile(1, 1);
    uint8_t event_id = 0;
    assert(N3D_RE_CommitPlayerWorldPosition(2 * 64 + 32, 1 * 64 + 32, &event_id));
    assert(n3d_player.tile_x == 2 && n3d_player.tile_y == 1);
    assert(event_id == N3D_ENTERED_TILE_EVENT);
    event_id = 0;
    assert(N3D_RE_CommitPlayerWorldPosition(2 * 64 + 40, 1 * 64 + 40, &event_id));
    assert(event_id == 0);

    N3D_RE_ResetRuntime();
    assert(N3D_RE_FirstFreeProjectileSlot() == 0);
    assert(N3D_RE_WeaponUsesProjectile(N3D_WEAPON_SINGLE_LASER));
    assert(N3D_RE_WeaponUsesProjectile(N3D_WEAPON_MAGIC_WAND));
    assert(!N3D_RE_WeaponUsesProjectile(N3D_WEAPON_SILVER_PISTOL));
    assert(N3D_RE_WeaponUsesProjectile(N3D_WEAPON_CONTINUOUS_LASER));

    n3d_projectile_sequence_offsets projectile_seq = {0};
    assert(N3D_RE_ProjectileSequenceOffsets(N3D_WEAPON_MAGIC_WAND, &projectile_seq));
    assert(projectile_seq.flight == 2 && projectile_seq.impact == 3);
    assert(!N3D_RE_ProjectileSequenceOffsets(N3D_WEAPON_SILVER_PISTOL, &projectile_seq));

    assert(N3D_RE_ProjectileHitsGuard(9, -9, 0, 0));
    assert(!N3D_RE_ProjectileHitsGuard(10, 0, 0, 0));
    assert(N3D_RE_ProjectileNeedsProjection(21, 0, 0, 0));
    assert(!N3D_RE_ProjectileNeedsProjection(20, -20, 0, 0));

    assert(N3D_RE_InitializeProjectile(
        0, N3D_WEAPON_MAGIC_WAND, 100, 200, 10));
    assert(n3d_projectiles[0].state == 1);
    assert(n3d_projectiles[0].object.animation_frame == 0);
    assert(n3d_projectiles[0].object.sequence_id == 12);
    assert(n3d_projectiles[0].object.flags == N3D_OBJECT_RUNTIME_PRESENT);
    assert(n3d_projectiles[0].object.object_class == 5);
    assert(n3d_projectiles[0].object.world_x == 100);
    assert(n3d_projectiles[0].object.world_y == 200);
    assert(n3d_projectiles[0].object.runtime_1a == 5);
    assert(N3D_RE_FirstFreeProjectileSlot() == 1);

    N3D_RE_AdvanceProjectileAnimation(0, 2);
    assert(n3d_projectiles[0].object.animation_frame == 1);
    N3D_RE_AdvanceProjectileAnimation(0, 2);
    assert(n3d_projectiles[0].object.animation_frame == 0);

    assert(N3D_RE_EnterProjectileImpact(
        0, N3D_WEAPON_MAGIC_WAND, 10));
    assert(n3d_projectiles[0].state == 2);
    assert(n3d_projectiles[0].object.sequence_id == 13);
    assert((n3d_projectiles[0].object.flags & 0x10) != 0);

    N3D_RE_AdvanceProjectileAnimation(0, 2);
    assert(n3d_projectiles[0].state == 2);
    assert(n3d_projectiles[0].object.animation_frame == 1);
    N3D_RE_AdvanceProjectileAnimation(0, 2);
    assert(n3d_projectiles[0].state == 0);

    for(int i = 0; i < N3D_MAX_PROJECTILES; ++i)
        n3d_projectiles[i].state = 1;
    assert(N3D_RE_FirstFreeProjectileSlot() == -1);

    assert(N3D_RE_ScalePlayerDamageByDifficulty(20, 0) == 40);
    assert(N3D_RE_ScalePlayerDamageByDifficulty(20, 1) == 20);
    assert(N3D_RE_ScalePlayerDamageByDifficulty(20, 2) == 10);
    assert(N3D_RE_ScaleEnemyDamageByDifficulty(20, 0) == 10);
    assert(N3D_RE_ScaleEnemyDamageByDifficulty(20, 2) == 40);

    assert(N3D_RE_ApplyClassWeaponDamageTransform(
        80, 0x0C, N3D_WEAPON_SINGLE_LASER, 0) == 10);
    assert(N3D_RE_ApplyClassWeaponDamageTransform(
        80, 0x0D, N3D_WEAPON_MAGIC_WAND, 0) == 40);
    assert(N3D_RE_ApplyClassWeaponDamageTransform(
        512, 0x0F, N3D_WEAPON_SINGLE_LASER, 0) == 2);
    assert(N3D_RE_ApplyClassWeaponDamageTransform(
        80, 0x0F, N3D_WEAPON_MAGIC_WAND, 0) == 40);
    assert(N3D_RE_ApplyClassWeaponDamageTransform(
        80, 0x12, N3D_WEAPON_MAGIC_WAND, 0) == 20);
    assert(N3D_RE_ApplyClassWeaponDamageTransform(
        80, 0x1A, N3D_WEAPON_MAGIC_WAND, 0) == 40);
    assert(N3D_RE_ApplyClassWeaponDamageTransform(
        80, 0x1A, N3D_WEAPON_SILVER_PISTOL, 0) == 0);
    assert(N3D_RE_ApplyClassWeaponDamageTransform(
        80, 0x16, N3D_WEAPON_MAGIC_WAND, 2) == 0);
    assert(N3D_RE_ApplyClassWeaponDamageTransform(
        80, 0x16, N3D_WEAPON_MAGIC_WAND, 3) == 3);

    n3d_damage_result damage_result = N3D_RE_ComputePlayerGuardDamage(
        90, 80, 0x0C, N3D_WEAPON_SINGLE_LASER, 1, 0, 0);
    assert(damage_result.raw_seed == 80);
    assert(damage_result.class_transformed == 10);
    assert(damage_result.difficulty_transformed == 10);
    assert(damage_result.stored_byte == 10);

    assert(N3D_RE_GuardScoreForClass(0x08) == 25);
    assert(N3D_RE_GuardScoreForClass(0x11) == 0);
    assert(N3D_RE_GuardScoreForClass(0x15) == -1000);
    assert(N3D_RE_GuardScoreForClass(0x16) == 1000);
    assert(N3D_RE_GuardScoreForClass(0x20) == 50);
    assert(N3D_RE_GuardScoreForClass(0x21) == 0);

    N3D_RE_ResetRuntime();
    assert(N3D_RE_RegisterGuardFromMap(128, 1, 1));
    n3d_guards[0].state = N3D_GUARD_STATE_07;
    assert(N3D_RE_ApplyGuardDamage(0, 10) == N3D_GUARD_HIT_PAIN);
    assert(n3d_guards[0].strength == 245);
    assert(n3d_guards[0].result_octant == 8);
    assert(n3d_guards[0].next_state == N3D_GUARD_STATE_07);
    assert(n3d_guards[0].state == N3D_GUARD_STATE_PAIN);

    N3D_RE_ResetRuntime();
    assert(N3D_RE_RegisterGuardFromMap(180, 1, 1));
    n3d_guards[0].state = N3D_GUARD_STATE_07;
    n3d_guards[0].strategy = 3;
    assert(N3D_RE_ApplyGuardDamage(0, 10) ==
           N3D_GUARD_HIT_SPECIAL_REACTION_REQUIRED);
    assert(n3d_guards[0].strength == 245);
    assert(n3d_guards[0].result_octant == 8);
    assert(n3d_guards[0].state == N3D_GUARD_STATE_07);

    N3D_RE_ResetRuntime();
    assert(N3D_RE_RegisterGuardFromMap(128, 1, 1));
    n3d_guards[0].strength = 10;
    assert(N3D_RE_ApplyGuardDamage(0, 10) == N3D_GUARD_HIT_KILLED);
    assert(n3d_guards[0].strength == 0);

    N3D_RE_ResetRuntime();
    assert(N3D_RE_RegisterGuardFromMap(176, 1, 1));
    assert(n3d_objects[0].object_class == 0x11);
    assert(N3D_RE_ApplyGuardDamage(0, 255) ==
           N3D_GUARD_HIT_DRACULA_TRANSFORMED);
    assert(n3d_objects[0].object_class == 0x14);
    assert(n3d_guards[0].strength == 0xFF);
    assert(n3d_guards[0].state == N3D_GUARD_STATE_08);
    assert(n3d_guards[0].next_state == N3D_GUARD_STATE_02);
    assert(n3d_guards[0].timer == 1);

    n3d_definition_record parsed_definition;
    assert(N3D_RE_ParseDefinitionLine(
        "18 O TOMB PUSH Tombstone pushable",
        &parsed_definition));
    assert(parsed_definition.id == 0x18);
    assert(strcmp(parsed_definition.visual_code, "O") == 0);
    assert(strcmp(parsed_definition.image_name, "TOMB") == 0);
    assert(strcmp(parsed_definition.class_name, "PUSH") == 0);
    assert(strcmp(parsed_definition.description, "Tombstone pushable") == 0);

    uint8_t known_mapped_type = 0;
    assert(N3D_RE_KnownWallMappedTypeForClass(
        "WARP_8", &known_mapped_type) && known_mapped_type == 0x14);
    assert(N3D_RE_KnownWallMappedTypeForClass(
        "WARP_S1", &known_mapped_type) && known_mapped_type == 0x15);
    assert(N3D_RE_KnownWallMappedTypeForClass(
        "WARP_L1", &known_mapped_type) && known_mapped_type == 0x19);
    assert(N3D_RE_KnownWallMappedTypeForClass(
        "TRIGGER1", &known_mapped_type) && known_mapped_type == 0x47);

    static const struct {
        const char* name;
        uint8_t mapped_type;
    } expected_wall_classes[] = {
        {"WALL",       0x01},
        {"REVWALL",    0x02},
        {"CONTROL",    0x03},
        {"ONE_SHOT",   0x07},
        {"SPECIAL1",   0x08},
        {"LEVEL_UP",   0x09},
        {"LEVEL_UP2",  0x0A},
        {"WARP_E1",    0x1D},
        {"WALL_EX",    0x2D},
        {"WALL_EX1",   0x2E},
        {"WALL_EX2",   0x2F},
        {"TURN",       0x41},
        {"RETREAT",    0x42},
        {"FLOOR",      0x44},
        {"SAFESPOT",   0x45},
        {"ACTIONSPOT", 0x46},
        {"TRIGGER1",   0x47},
        {"TRIGGER2",   0x48}
    };

    for(size_t i = 0;
        i < sizeof(expected_wall_classes) / sizeof(expected_wall_classes[0]);
        ++i)
    {
        assert(N3D_RE_KnownWallMappedTypeForClass(
            expected_wall_classes[i].name, &known_mapped_type));
        assert(known_mapped_type == expected_wall_classes[i].mapped_type);
    }

    static const struct {
        const char* name;
        uint8_t mapped_type;
    } expected_door_classes[] = {
        {"DOORV",   0x31},
        {"DOORH",   0x32},
        {"DOORVL",  0x33},
        {"DOORHL",  0x34},
        {"DOORVL2", 0x35},
        {"DOORHL2", 0x36},
        {"DOORVL3", 0x37},
        {"DOORHL3", 0x38},
        {"DOORVI",  0x39},
        {"DOORHI",  0x3A},
        {"DOORVR",  0x3B},
        {"DOORHR",  0x3C},
        {"DOORVC",  0x3F},
        {"DOORHC",  0x40}
    };

    for(size_t i = 0;
        i < sizeof(expected_door_classes) / sizeof(expected_door_classes[0]);
        ++i)
    {
        assert(N3D_RE_KnownWallMappedTypeForClass(
            expected_door_classes[i].name, &known_mapped_type));
        assert(known_mapped_type == expected_door_classes[i].mapped_type);
    }

    assert(!N3D_RE_KnownWallMappedTypeForClass(
        "DOOR_UNRESOLVED_3D", &known_mapped_type));

    uint8_t known_property_flags = 0;
    assert(N3D_RE_KnownWallPropertyForClass(
        "DOORV", &known_property_flags) && known_property_flags == 0x0B);
    assert(N3D_RE_KnownWallPropertyForClass(
        "DOORVR", &known_property_flags) && known_property_flags == 0x0B);
    assert(N3D_RE_KnownWallPropertyForClass(
        "DOORVC", &known_property_flags) && known_property_flags == 0x0B);
    assert(!N3D_RE_KnownWallPropertyForClass(
        "UNRESOLVED_DOOR", &known_property_flags));

    static const struct {
        const char* name;
        uint8_t mapped_type;
    } expected_object_classes[] = {
        {"NULL",      0x00},
        {"START",     0x02},
        {"SECRET",    0x03},
        {"CAUSTIC",   0x07},
        {"SAFE",      0x26},
        {"TRUNK",     0x27},
        {"PUSH",      0x28},
        {"ACTION",    0x29},
        {"PERMEABLE", 0x2A},
        {"DUMB",      0x2B},
        {"ELEVATED",  0x2E},
        {"KEY",       0x2F},
        {"IDCARD",    0x30},
        {"FOOD",      0x33},
        {"WEAPON",    0x36},
        {"AMMO",      0x39},
        {"CRYSTALB",  0x3A},
        {"MAGICEYE",  0x3B},
        {"PENTAGRAM", 0x3C},
        {"SCROLL",    0x3D},
        {"GUARD1",    0x08},
        {"GUARD10",   0x11},
        {"GUARD14",   0x15},
        {"GUARD26",   0x21}
    };

    for(size_t i = 0;
        i < sizeof(expected_object_classes) / sizeof(expected_object_classes[0]);
        ++i)
    {
        assert(N3D_RE_KnownObjectMappedTypeForClass(
            expected_object_classes[i].name, &known_mapped_type));
        assert(known_mapped_type == expected_object_classes[i].mapped_type);
    }

    assert(!N3D_RE_KnownObjectMappedTypeForClass(
        "GUARD0", &known_mapped_type));
    assert(!N3D_RE_KnownObjectMappedTypeForClass(
        "GUARD27", &known_mapped_type));
    assert(!N3D_RE_KnownObjectMappedTypeForClass(
        "UNRESOLVED_CLASS", &known_mapped_type));

    FILE* walls_test = fopen("WALLS.1", "wb");
    assert(walls_test != NULL);
    fputs("01 W WALLIMG WALL Ordinary wall\n", walls_test);
    fputs("02 W REVIMG REVWALL Reversible wall\n", walls_test);
    fputs("12 W SPECIALIMG SPECIAL1 Scripted wall\n", walls_test);
    fputs("4E W CONTROLIMG CONTROL Remote control\n", walls_test);
    fputs("53 W EXIMG WALL_EX1 Explodable wall\n", walls_test);
    fputs("54 W ONESHOTIMG ONE_SHOT One shot wall\n", walls_test);
    fputs("70 D DOORIMG DOORV Ordinary vertical door\n", walls_test);
    fputs("71 W UNKNOWNIMG UNKNOWN_WALL Unresolved wall\n", walls_test);
    fputs("72 D CURTAINIMG DOORVC Vertical curtain door\n", walls_test);
    fputs("90 W WARPIMG WARP_1 Paired warp\n", walls_test);
    fputs("91 W KEYIMG WARP_L4 Yellow key gate\n", walls_test);
    fputs("92 W LEVELIMG LEVEL_UP Level exit\n", walls_test);
    fputs("93 W MIRRORIMG WARP_S2 Other Side mirror\n", walls_test);
    fputs("94 W TRIGIMG TRIGGER2 Trigger two\n", walls_test);
    fputs("95 W SWIRLIMG WARP_S1 Swirling mirror\n", walls_test);
    fputs("96 W LEVEL2IMG LEVEL_UP2 Skip level\n", walls_test);
    fputs("97 W REDKEYIMG WARP_L1 Red key gate\n", walls_test);
    fputs("B7 W ACTIONIMG ACTIONSPOT Action spot\n", walls_test);
    fputs("BA W FLOORIMG FLOOR Floor marker\n", walls_test);
    fputs("E1 W RETREATIMG RETREAT Retreat marker\n", walls_test);
    fputs("EA W TURNIMG TURN Turn marker\n", walls_test);
    fputs("F3 W SAFEIMG SAFESPOT Safe spot\n", walls_test);
    fclose(walls_test);

    FILE* objects_test = fopen("OBJECTS.1", "wb");
    assert(objects_test != NULL);
    fputs("00 O NONE NULL Nothing\n", objects_test);
    fputs("01 O STARTIMG START Start north\n", objects_test);
    fputs("05 O KEYIMG KEY Red key\n", objects_test);
    fputs("09 O CARDIMG IDCARD Red ID card\n", objects_test);
    fputs("0B O DESKIMG DUMB Office desk\n", objects_test);
    fputs("0D O LAMPIMG ELEVATED Ceiling lamp\n", objects_test);
    fputs("12 O FOODIMG FOOD Red potion\n", objects_test);
    fputs("18 O TOMB PUSH Tombstone pushable\n", objects_test);
    fputs("19 O EYEIMG MAGICEYE Magic eye\n", objects_test);
    fputs("1A O BALLIMG CRYSTALB Crystal ball\n", objects_test);
    fputs("1F O PENTIMG PENTAGRAM Red pentagram\n", objects_test);
    fputs("25 O WEAPONIMG WEAPON Plasma gun\n", objects_test);
    fputs("29 O AMMOIMG AMMO Silver bullets\n", objects_test);
    fputs("2A O LASERAMMO AMMO Plasma power cell\n", objects_test);
    fputs("2B O WANDAMMO AMMO Spell book\n", objects_test);
    fputs("2C O SCROLLIMG SCROLL Code scroll\n", objects_test);
    fputs("3B O FIREIMG CAUSTIC Fire\n", objects_test);
    fputs("40 O TORCHIMG PERMEABLE Flaming torch\n", objects_test);
    fputs("45 O RADIOIMG ACTION Radio\n", objects_test);
    fputs("62 O SECRETIMG SECRET Secret panel\n", objects_test);
    fputs("80 O BATIMG GUARD1 Bat north\n", objects_test);
    fputs("8C O DANCERIMG GUARD26 Dancers\n", objects_test);
    fputs("D2 O SAFEIMG SAFE Safe\n", objects_test);
    fputs("D8 O TRUNKIMG TRUNK Trunk\n", objects_test);
    fclose(objects_test);

    assert(N3D_RE_LoadEpisodeDefinitions(1));
    assert(n3d_wall_definitions.count == 22);
    assert(n3d_object_definitions.count == 24);

    n3d_mapping_coverage wall_mapping_coverage =
        N3D_RE_WallMappingCoverage();
    n3d_mapping_coverage object_mapping_coverage =
        N3D_RE_ObjectMappingCoverage();
    assert(wall_mapping_coverage.total == 22);
    assert(wall_mapping_coverage.known == 21);
    assert(wall_mapping_coverage.unknown == 1);
    assert(object_mapping_coverage.total == 24);
    assert(object_mapping_coverage.known == 24);
    assert(object_mapping_coverage.unknown == 0);

    const n3d_definition_record* push_def =
        N3D_RE_FindDefinition(&n3d_object_definitions, 0x18);
    assert(push_def != NULL);
    assert(strcmp(push_def->class_name, "PUSH") == 0);
    assert(N3D_RE_ObjectMappingKnown(0x18));
    assert(n3d_object_mapped_type[0x18] == 0x28);
    assert(n3d_object_property_resolved[0x18] == 0x03);

    assert(N3D_RE_ObjectMappingKnown(0x80));
    assert(n3d_object_mapped_type[0x80] == 0x08);
    assert(n3d_object_property_resolved[0x80] == 0x0B);

    assert(N3D_RE_ObjectMappingKnown(0x62));
    assert(n3d_object_mapped_type[0x62] == 0x03);
    assert(n3d_object_property_resolved[0x62] == 0x00);

    assert(N3D_RE_ObjectMappingKnown(0x05));
    assert(n3d_object_mapped_type[0x05] == 0x2F);
    assert(n3d_object_property_resolved[0x05] == 0x05);

    assert(N3D_RE_ObjectMappingKnown(0x25));
    assert(n3d_object_mapped_type[0x25] == 0x36);
    assert(n3d_object_property_resolved[0x25] == 0x05);

    assert(N3D_RE_ObjectMappingKnown(0x29));
    assert(n3d_object_mapped_type[0x29] == 0x39);
    assert(n3d_object_property_resolved[0x29] == 0x05);

    assert(N3D_RE_ObjectMappingKnown(0x1F));
    assert(n3d_object_mapped_type[0x1F] == 0x3C);
    assert(n3d_object_property_resolved[0x1F] == 0x05);

    assert(N3D_RE_ObjectMappingKnown(0x8C));
    assert(n3d_object_mapped_type[0x8C] == 0x21);
    assert(n3d_object_property_resolved[0x8C] == 0x0B);

    assert(N3D_RE_WallMappingKnown(0x01));
    assert(N3D_RE_WallPropertyKnown(0x01));
    assert(n3d_wall_mapped_type[0x01] == 0x01);
    assert(n3d_wall_property_resolved[0x01] == 0x07);

    assert(N3D_RE_WallMappingKnown(0x02));
    assert(n3d_wall_mapped_type[0x02] == 0x02);
    assert(n3d_wall_property_resolved[0x02] == 0x07);

    assert(N3D_RE_WallMappingKnown(0x12));
    assert(n3d_wall_mapped_type[0x12] == 0x08);
    assert(n3d_wall_property_resolved[0x12] == 0x07);

    assert(N3D_RE_WallMappingKnown(0x4E));
    assert(n3d_wall_mapped_type[0x4E] == 0x03);
    assert(n3d_wall_property_resolved[0x4E] == 0x07);

    assert(N3D_RE_WallMappingKnown(0x53));
    assert(n3d_wall_mapped_type[0x53] == 0x2E);
    assert(n3d_wall_property_resolved[0x53] == 0x17);

    assert(N3D_RE_WallMappingKnown(0x70));
    assert(N3D_RE_WallPropertyKnown(0x70));
    assert(n3d_wall_mapped_type[0x70] == 0x31);
    assert(n3d_wall_property_resolved[0x70] == 0x0B);

    assert(!N3D_RE_WallMappingKnown(0x71));
    assert(!N3D_RE_WallPropertyKnown(0x71));

    assert(N3D_RE_WallMappingKnown(0x72));
    assert(N3D_RE_WallPropertyKnown(0x72));
    assert(n3d_wall_mapped_type[0x72] == 0x3F);
    assert(n3d_wall_property_resolved[0x72] == 0x0B);

    assert(N3D_RE_WallMappingKnown(0x90));
    assert(n3d_wall_mapped_type[0x90] == 0x0D);
    assert(n3d_wall_property_resolved[0x90] == 0x07);

    assert(N3D_RE_WallMappingKnown(0x91));
    assert(n3d_wall_mapped_type[0x91] == 0x1C);
    assert(n3d_wall_property_resolved[0x91] == 0x07);

    assert(N3D_RE_WallMappingKnown(0x92));
    assert(n3d_wall_mapped_type[0x92] == 0x09);
    assert(n3d_wall_property_resolved[0x92] == 0x07);

    assert(N3D_RE_WallMappingKnown(0x93));
    assert(n3d_wall_mapped_type[0x93] == 0x16);
    assert(n3d_wall_property_resolved[0x93] == 0x07);

    assert(N3D_RE_WallMappingKnown(0x94));
    assert(n3d_wall_mapped_type[0x94] == 0x48);
    assert(n3d_wall_property_resolved[0x94] == 0x40);

    assert(N3D_RE_WallMappingKnown(184));
    assert(n3d_wall_mapped_type[184] == 0x47);
    assert(n3d_wall_property_resolved[184] == 0x40);

    assert(N3D_RE_WallMappingKnown(185));
    assert(n3d_wall_mapped_type[185] == 0x48);
    assert(n3d_wall_property_resolved[185] == 0x40);

    assert(N3D_RE_WallMappingKnown(254));
    assert(n3d_wall_mapped_type[254] == 0x2E);
    assert(n3d_wall_property_resolved[254] == 0x17);

    assert(N3D_RE_WallMappingKnown(255));
    assert(n3d_wall_mapped_type[255] == 0x2F);
    assert(n3d_wall_property_resolved[255] == 0x17);

    /*
     * Full map-object instantiation now follows OBJECTS class/property mapping:
     * KEY -> OBJECT, PUSH -> OBJECT+PUSH, SECRET -> PANEL,
     * GUARD1/GUARD26 -> OBJECT+GUARD.
     */
    uint8_t object_payload[N3D_MAP_LEVEL_BYTES] = {0};
    const int key_cell = 2 * N3D_MAP_WIDTH + 2;
    const int push_cell = 3 * N3D_MAP_WIDTH + 3;
    const int panel_cell = 4 * N3D_MAP_WIDTH + 4;
    const int bat_runtime_cell = 5 * N3D_MAP_WIDTH + 5;
    const int dancers_cell = 6 * N3D_MAP_WIDTH + 6;

    object_payload[key_cell * N3D_MAP_CELL_BYTES + 1] = 0x05;
    object_payload[push_cell * N3D_MAP_CELL_BYTES + 1] = 0x18;
    object_payload[panel_cell * N3D_MAP_CELL_BYTES + 1] = 0x62;
    object_payload[bat_runtime_cell * N3D_MAP_CELL_BYTES + 1] = 0x80;
    object_payload[dancers_cell * N3D_MAP_CELL_BYTES + 1] = 0x8C;

    assert(N3D_RE_LoadMapPayload(object_payload, sizeof(object_payload)));
    assert(n3d_object_count == 4);
    assert(n3d_guard_count == 2);
    assert(n3d_push_count == 1);
    assert(n3d_panel_count == 1);

    int key_object_slot = N3D_RE_FindObjectSlotByCell(2, 2);
    int push_object_slot = N3D_RE_FindObjectSlotByCell(3, 3);
    int bat_object_slot = N3D_RE_FindObjectSlotByCell(5, 5);
    int dancers_object_slot = N3D_RE_FindObjectSlotByCell(6, 6);

    assert(key_object_slot >= 0);
    assert(push_object_slot >= 0);
    assert(bat_object_slot >= 0);
    assert(dancers_object_slot >= 0);
    assert(N3D_RE_FindObjectSlotByCell(4, 4) == -1);

    assert(n3d_objects[key_object_slot].object_class == 0x2F);
    assert(n3d_objects[key_object_slot].flags == 0x05);
    assert(n3d_objects[key_object_slot].world_x == 2 * 64 + 32);
    assert(n3d_objects[key_object_slot].world_y == 2 * 64 + 32);

    assert(n3d_objects[push_object_slot].object_class == 0x28);
    assert(n3d_objects[push_object_slot].flags == 0x03);
    int push_runtime_slot =
        N3D_RE_FindPushSlotByObject((uint16_t)push_object_slot);
    assert(push_runtime_slot >= 0);
    assert(n3d_pushes[push_runtime_slot].object_index ==
           (uint16_t)push_object_slot);
    assert(n3d_pushes[push_runtime_slot].steps_remaining == 0);

    int panel_runtime_slot = N3D_RE_FindPanelSlotByCell(4, 4);
    assert(panel_runtime_slot >= 0);
    assert(N3D_RE_PanelActivation(&n3d_panels[panel_runtime_slot]) == 0);

    assert(n3d_objects[bat_object_slot].object_class == 0x08);
    assert(n3d_objects[bat_object_slot].flags == 0x0B);
    assert(n3d_guards[n3d_objects[bat_object_slot].guard_index].object_slot ==
           (uint16_t)bat_object_slot);

    assert(n3d_objects[dancers_object_slot].object_class == 0x21);
    assert(n3d_objects[dancers_object_slot].flags == 0x0B);
    const n3d_guard_record* dancers_guard =
        &n3d_guards[n3d_objects[dancers_object_slot].guard_index];
    assert(dancers_guard->object_slot == (uint16_t)dancers_object_slot);
    assert(dancers_guard->state == 0);
    assert(dancers_guard->next_state == 0);

    uint8_t door_payload[N3D_MAP_LEVEL_BYTES] = {0};
    const int doorv_cell = 5 * N3D_MAP_WIDTH + 4;
    const int doorvc_cell = 7 * N3D_MAP_WIDTH + 6;
    door_payload[doorv_cell * N3D_MAP_CELL_BYTES] = 0x70;
    door_payload[doorvc_cell * N3D_MAP_CELL_BYTES] = 0x72;

    assert(N3D_RE_LoadMapPayload(door_payload, sizeof(door_payload)));
    assert(n3d_door_count == 2);
    assert(N3D_RE_FindDoorSlotByCell(4, 5) >= 0);
    assert(N3D_RE_FindDoorSlotByCell(6, 7) >= 0);
    assert(!N3D_RE_DoorCellStateKnown(4, 5));
    assert(!N3D_RE_DoorCellStateKnown(6, 7));
    assert(!N3D_RE_DoorCellAllowsPassage(4, 5));
    assert(!N3D_RE_DoorCellAllowsPassage(6, 7));

    assert(N3D_RE_SetDoorCellState(4, 5, 0));
    assert(N3D_RE_DoorCellStateKnown(4, 5));
    assert(N3D_RE_DoorCellAllowsPassage(4, 5));

    assert(N3D_RE_SetDoorCellState(6, 7, 2));
    assert(N3D_RE_DoorCellStateKnown(6, 7));
    assert(!N3D_RE_DoorCellAllowsPassage(6, 7));

    assert(N3D_RE_SetDoorCellState(6, 7, 4));
    assert(N3D_RE_DoorCellAllowsPassage(6, 7));

    N3D_RE_ResetRuntime();
    n3d_map[0].wall = 0;
    n3d_map[0].object = 0;
    n3d_map[1].wall = 0;
    n3d_map[1].object = 0;

    n3d_resolved_collision_result resolved_collision =
        N3D_RE_TestResolvedLeadingEdgePair(0, 0, 1, 0, 1, NULL);
    assert(resolved_collision.resolved);
    assert(resolved_collision.step == 1);

    n3d_map[0].wall = 0x71;
    resolved_collision =
        N3D_RE_TestResolvedLeadingEdgePair(0, 0, 1, 0, 1, NULL);
    assert(!resolved_collision.resolved);
    assert(resolved_collision.step == 0);

    n3d_map[0].wall = 0x70;
    resolved_collision =
        N3D_RE_TestResolvedLeadingEdgePair(0, 0, 1, 0, 1, NULL);
    assert(resolved_collision.resolved);
    assert(resolved_collision.step == 0);

    int verified_door_open = 1;
    n3d_collision_callbacks verified_door_callbacks = {
        test_door_passable,
        NULL,
        NULL,
        &verified_door_open
    };
    resolved_collision =
        N3D_RE_TestResolvedLeadingEdgePair(
            0, 0, 1, 0, 1, &verified_door_callbacks);
    assert(resolved_collision.resolved);
    assert(resolved_collision.step == 1);

    n3d_map[0].wall = 184;
    int resolved_touch_count = 0;
    n3d_collision_callbacks resolved_callbacks = {
        NULL,
        test_touch,
        NULL,
        &resolved_touch_count
    };
    resolved_collision =
        N3D_RE_TestResolvedLeadingEdgePair(
            0, 0, 1, 0, 1, &resolved_callbacks);
    assert(resolved_collision.resolved);
    assert(resolved_collision.step == 1);
    assert(resolved_touch_count == 1);

    n3d_map[0].wall = 0;
    n3d_map[0].object = 0x18;
    resolved_collision =
        N3D_RE_TestResolvedLeadingEdgePair(0, 0, 1, 0, 1, NULL);
    assert(resolved_collision.resolved);
    assert(resolved_collision.step == 0);

    n3d_map[0].object = 0x80;
    resolved_collision =
        N3D_RE_TestResolvedLeadingEdgePair(0, 0, 1, 0, 1, NULL);
    assert(resolved_collision.resolved);
    assert(resolved_collision.step == 0);

    remove("WALLS.1");
    remove("OBJECTS.1");

    assert(sizeof(n3d_door_record) == 22);
    N3D_RE_ResetDoors();
    assert(n3d_door_count == 0);

    n3d_door_record door = {{0}};
    N3D_RE_SetDoorStateValue(&door, 0);
    assert(N3D_RE_DoorRecordAllowsPassage(&door));
    N3D_RE_SetDoorStateValue(&door, 4);
    assert(N3D_RE_DoorRecordAllowsPassage(&door));
    N3D_RE_SetDoorStateValue(&door, 1);
    assert(!N3D_RE_DoorRecordAllowsPassage(&door));
    N3D_RE_SetDoorStateValue(&door, 2);
    assert(!N3D_RE_DoorRecordAllowsPassage(&door));
    N3D_RE_SetDoorStateValue(&door, 3);
    assert(!N3D_RE_DoorRecordAllowsPassage(&door));

    memset(&door, 0, sizeof(door));
    N3D_RE_SetDoorStateValue(&door, 1);
    assert(N3D_RE_ApplyRemoteDoorCommand(&door, 0x1E));
    assert(N3D_RE_DoorStateValue(&door) == 2);
    assert(N3D_RE_DoorTransitionFlag(&door) == 1);

    memset(&door, 0, sizeof(door));
    N3D_RE_SetDoorStateValue(&door, 3);
    assert(N3D_RE_ApplyRemoteDoorCommand(&door, 0x1E));
    assert(N3D_RE_DoorStateValue(&door) == 2);
    assert(N3D_RE_DoorTransitionFlag(&door) == 1);

    memset(&door, 0, sizeof(door));
    N3D_RE_SetDoorStateValue(&door, 0);
    assert(N3D_RE_ApplyRemoteDoorCommand(&door, 0x1F));
    assert(N3D_RE_DoorStateValue(&door) == 3);
    assert(N3D_RE_DoorTransitionFlag(&door) == 1);

    memset(&door, 0, sizeof(door));
    N3D_RE_SetDoorStateValue(&door, 2);
    assert(N3D_RE_ApplyRemoteDoorCommand(&door, 0x1F));
    assert(N3D_RE_DoorStateValue(&door) == 3);
    assert(N3D_RE_DoorTransitionFlag(&door) == 1);

    memset(&door, 0, sizeof(door));
    N3D_RE_SetDoorStateValue(&door, 0);
    assert(!N3D_RE_ApplyRemoteDoorCommand(&door, 0x1E));
    assert(N3D_RE_DoorStateValue(&door) == 0);
    assert(N3D_RE_DoorTransitionFlag(&door) == 0);
    assert(!N3D_RE_ApplyRemoteDoorCommand(&door, 0x20));

    n3d_door_count = 1;
    n3d_doors[0] = door;
    N3D_RE_ResetRuntime();
    assert(n3d_door_count == 0);
    assert(N3D_RE_DoorStateValue(&n3d_doors[0]) == 0);
    assert(N3D_RE_DoorTransitionFlag(&n3d_doors[0]) == 0);

    assert(sizeof(n3d_panel_record) == 22);
    assert(sizeof(n3d_push_record) == 6);

    n3d_panel_record panel = {{0}};
    assert(N3D_RE_PanelActivation(&panel) == 0);
    N3D_RE_ActivatePanelUse(&panel);
    assert(N3D_RE_PanelActivation(&panel) == 2);
    N3D_RE_SetPanelActivation(&panel, 1);
    assert(N3D_RE_PanelActivation(&panel) == 1);

    n3d_push_direction push_dir = N3D_RE_PushDirectionForOctant(0);
    assert(push_dir.dx == 0 && push_dir.dy == -8);
    push_dir = N3D_RE_PushDirectionForOctant(1);
    assert(push_dir.dx == 8 && push_dir.dy == 0);
    push_dir = N3D_RE_PushDirectionForOctant(4);
    assert(push_dir.dx == 0 && push_dir.dy == 8);
    push_dir = N3D_RE_PushDirectionForOctant(6);
    assert(push_dir.dx == -8 && push_dir.dy == 0);

    n3d_push_record push = {0};
    assert(N3D_RE_CanStartPush(&push, 0));
    assert(!N3D_RE_CanStartPush(&push, 0x02));
    assert(N3D_RE_BeginPush(&push, 7, 1));
    assert(push.object_index == 7);
    assert(push.delta_x == 8 && push.delta_y == 0);
    assert(push.steps_remaining == 8);
    assert(!N3D_RE_BeginPush(&push, 8, 1));

    int push_total_x = 0;
    int push_total_y = 0;
    for(int i = 0; i < 8; ++i)
    {
        int8_t dx = 0, dy = 0;
        assert(N3D_RE_StepPush(&push, &dx, &dy));
        push_total_x += dx;
        push_total_y += dy;
    }
    assert(push_total_x == 64 && push_total_y == 0);
    assert(push.steps_remaining == 0);
    assert(!N3D_RE_StepPush(&push, NULL, NULL));

    n3d_panel_count = 1;
    n3d_push_count = 1;
    n3d_panels[0] = panel;
    n3d_pushes[0] = push;
    N3D_RE_ResetRuntime();
    assert(n3d_panel_count == 0);
    assert(n3d_push_count == 0);
    assert(N3D_RE_PanelActivation(&n3d_panels[0]) == 0);
    assert(n3d_pushes[0].steps_remaining == 0);

    N3D_RE_ResetUseLatch();
    assert(!N3D_RE_UseRisingEdge(0));
    assert(N3D_RE_UseRisingEdge(1));
    assert(!N3D_RE_UseRisingEdge(1));
    assert(!N3D_RE_UseRisingEdge(0));
    assert(N3D_RE_UseRisingEdge(1));

    uint8_t use_x = 0, use_y = 0;
    assert(N3D_RE_AdjacentUseCell(10, 10, 0, &use_x, &use_y));
    assert(use_x == 10 && use_y == 9);
    assert(N3D_RE_AdjacentUseCell(10, 10, 1, &use_x, &use_y));
    assert(use_x == 11 && use_y == 10);
    assert(N3D_RE_AdjacentUseCell(10, 10, 2, &use_x, &use_y));
    assert(use_x == 11 && use_y == 10);
    assert(N3D_RE_AdjacentUseCell(10, 10, 3, &use_x, &use_y));
    assert(use_x == 10 && use_y == 11);
    assert(N3D_RE_AdjacentUseCell(10, 10, 4, &use_x, &use_y));
    assert(use_x == 10 && use_y == 11);
    assert(N3D_RE_AdjacentUseCell(10, 10, 5, &use_x, &use_y));
    assert(use_x == 9 && use_y == 10);
    assert(N3D_RE_AdjacentUseCell(10, 10, 6, &use_x, &use_y));
    assert(use_x == 9 && use_y == 10);
    assert(N3D_RE_AdjacentUseCell(10, 10, 7, &use_x, &use_y));
    assert(use_x == 10 && use_y == 9);
    assert(!N3D_RE_AdjacentUseCell(0, 0, 0, &use_x, &use_y));
    assert(!N3D_RE_AdjacentUseCell(0, 0, 6, &use_x, &use_y));

    N3D_RE_InitPlayerAtTile(10, 10);
    N3D_RE_ResetRuntime();
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;

    const int use_east_cell = 10 * N3D_MAP_WIDTH + 11;
    n3d_map[use_east_cell].wall = 0;
    n3d_map[use_east_cell].object = 0;

    n3d_use_target use_target = N3D_RE_ClassifyUseTarget(1);
    assert(use_target.kind == N3D_USE_NONE);
    assert(use_target.x == 11 && use_target.y == 10);

    n3d_map[use_east_cell].wall = 0x71;
    use_target = N3D_RE_ClassifyUseTarget(1);
    assert(use_target.kind == N3D_USE_UNRESOLVED);

    n3d_map[use_east_cell].wall = 0x70;
    use_target = N3D_RE_ClassifyUseTarget(1);
    assert(use_target.kind == N3D_USE_DYNAMIC_DOOR);
    assert(use_target.mapped_wall_type == 0x31);

    /* Prove the property-only layer still works independently of exact mapping. */
    n3d_wall_mapping_known[0x73] = 0;
    n3d_wall_property_known[0x73] = 1;
    n3d_wall_mapped_type[0x73] = N3D_MAPPED_TYPE_UNKNOWN;
    n3d_wall_property_resolved[0x73] = 0x0B;
    n3d_map[use_east_cell].wall = 0x73;
    use_target = N3D_RE_ClassifyUseTarget(1);
    assert(use_target.kind == N3D_USE_DYNAMIC_DOOR);
    assert(use_target.mapped_wall_type == 0);

    n3d_map[use_east_cell].wall = 0x72;
    use_target = N3D_RE_ClassifyUseTarget(1);
    assert(use_target.kind == N3D_USE_DYNAMIC_DOOR);
    assert(use_target.mapped_wall_type == 0x3F);

    n3d_map[use_east_cell].wall = 184;
    use_target = N3D_RE_ClassifyUseTarget(1);
    assert(use_target.kind == N3D_USE_MAPPED_WALL);
    assert(use_target.mapped_wall_type == 0x47);

    n3d_map[use_east_cell].wall = 0;
    n3d_object_mapping_known[98] = 1;
    n3d_object_property_known[98] = 1;
    n3d_object_mapped_type[98] = 0x03;
    n3d_object_property_resolved[98] =
        N3D_RE_ObjectPropertiesForMappedType(0x03);
    n3d_map[use_east_cell].object = 98;
    use_target = N3D_RE_ClassifyUseTarget(1);
    assert(use_target.kind == N3D_USE_PANEL);

    n3d_map[use_east_cell].object = 0x18;
    use_target = N3D_RE_ClassifyUseTarget(1);
    assert(use_target.kind == N3D_USE_PUSH);

    n3d_map[use_east_cell].object = 0x80;
    use_target = N3D_RE_ClassifyUseTarget(1);
    assert(use_target.kind == N3D_USE_MAPPED_OBJECT);
    assert(use_target.mapped_object_type == 0x08);

    /*
     * Verified object-touch subset: key/card/pentagram masks and three ammo
     * pools. Object removal/SFX/score remain caller-side/deferred.
     */
    uint8_t pickup_payload[N3D_MAP_LEVEL_BYTES] = {0};
    const int pickup_key_cell = 1 * N3D_MAP_WIDTH + 1;
    const int pickup_card_cell = 2 * N3D_MAP_WIDTH + 2;
    const int pickup_pentagram_cell = 3 * N3D_MAP_WIDTH + 3;
    const int pickup_silver_cell = 4 * N3D_MAP_WIDTH + 4;
    const int pickup_laser_cell = 5 * N3D_MAP_WIDTH + 5;
    const int pickup_wand_cell = 6 * N3D_MAP_WIDTH + 6;
    const int pickup_deferred_cell = 7 * N3D_MAP_WIDTH + 7;

    pickup_payload[pickup_key_cell * N3D_MAP_CELL_BYTES + 1] = 0x05;
    pickup_payload[pickup_card_cell * N3D_MAP_CELL_BYTES + 1] = 0x09;
    pickup_payload[pickup_pentagram_cell * N3D_MAP_CELL_BYTES + 1] = 0x1F;
    pickup_payload[pickup_silver_cell * N3D_MAP_CELL_BYTES + 1] = 0x29;
    pickup_payload[pickup_laser_cell * N3D_MAP_CELL_BYTES + 1] = 0x2A;
    pickup_payload[pickup_wand_cell * N3D_MAP_CELL_BYTES + 1] = 0x2B;
    pickup_payload[pickup_deferred_cell * N3D_MAP_CELL_BYTES + 1] = 0x2C;

    assert(N3D_RE_LoadMapPayload(pickup_payload, sizeof(pickup_payload)));
    n3d_player.colored_keys = 0;
    n3d_player.id_cards = 0;
    n3d_player.pentagrams = 0;
    n3d_player.silver_ammo = 99;
    n3d_player.laser_ammo = 100;
    n3d_player.wand_ammo = 90;

    n3d_pickup_result pickup_result =
        N3D_RE_ApplyPickupAtCell(1, 1);
    assert(pickup_result.kind == N3D_PICKUP_KEY_GRANTED);
    assert(pickup_result.variant_index == 0);
    assert(N3D_RE_PickupResultAccepted(&pickup_result));
    assert(n3d_player.colored_keys == 0x01);
    int pickup_key_object_slot = N3D_RE_FindObjectSlotByCell(1, 1);
    assert(pickup_key_object_slot >= 0);
    assert((n3d_objects[pickup_key_object_slot].flags &
            N3D_OBJECT_RUNTIME_PRESENT) == 0);

    pickup_result = N3D_RE_ApplyPickupAtCell(1, 1);
    assert(pickup_result.kind == N3D_PICKUP_INACTIVE);
    assert(!N3D_RE_PickupResultAccepted(&pickup_result));
    assert(n3d_player.colored_keys == 0x01);

    pickup_result = N3D_RE_ApplyPickupAtCell(2, 2);
    assert(pickup_result.kind == N3D_PICKUP_IDCARD_GRANTED);
    assert(pickup_result.variant_index == 0);
    assert(n3d_player.id_cards == 0x01);

    pickup_result = N3D_RE_ApplyPickupAtCell(3, 3);
    assert(pickup_result.kind == N3D_PICKUP_PENTAGRAM_GRANTED);
    assert(pickup_result.variant_index == 0);
    assert(n3d_player.pentagrams == 0x01);

    pickup_result = N3D_RE_ApplyPickupAtCell(4, 4);
    assert(pickup_result.kind == N3D_PICKUP_AMMO_ADDED);
    assert(pickup_result.ammo_pool == N3D_AMMO_POOL_SILVER);
    assert(pickup_result.value_before == 99);
    assert(pickup_result.value_after == 119);
    assert(pickup_result.accepted == 1);
    assert(n3d_player.silver_ammo == 119);
    int silver_pickup_slot = N3D_RE_FindObjectSlotByCell(4, 4);
    assert(silver_pickup_slot >= 0);
    assert((n3d_objects[silver_pickup_slot].flags &
            N3D_OBJECT_RUNTIME_PRESENT) == 0);

    pickup_result = N3D_RE_ApplyPickupAtCell(5, 5);
    assert(pickup_result.kind == N3D_PICKUP_AMMO_AT_THRESHOLD);
    assert(pickup_result.ammo_pool == N3D_AMMO_POOL_LASER);
    assert(pickup_result.value_before == 100);
    assert(pickup_result.value_after == 100);
    assert(pickup_result.accepted == 0);
    assert(n3d_player.laser_ammo == 100);
    int laser_pickup_slot = N3D_RE_FindObjectSlotByCell(5, 5);
    assert(laser_pickup_slot >= 0);
    assert((n3d_objects[laser_pickup_slot].flags &
            N3D_OBJECT_RUNTIME_PRESENT) != 0);

    pickup_result = N3D_RE_ApplyPickupAtCell(5, 5);
    assert(pickup_result.kind == N3D_PICKUP_AMMO_AT_THRESHOLD);
    assert(n3d_player.laser_ammo == 100);
    assert((n3d_objects[laser_pickup_slot].flags &
            N3D_OBJECT_RUNTIME_PRESENT) != 0);

    pickup_result = N3D_RE_ApplyPickupAtCell(6, 6);
    assert(pickup_result.kind == N3D_PICKUP_AMMO_ADDED);
    assert(pickup_result.ammo_pool == N3D_AMMO_POOL_WAND);
    assert(pickup_result.value_before == 90);
    assert(pickup_result.value_after == 110);
    assert(n3d_player.wand_ammo == 110);

    pickup_result = N3D_RE_ApplyPickupAtCell(7, 7);
    assert(pickup_result.kind == N3D_PICKUP_DEFERRED);
    assert(pickup_result.object_class == 0x3D);

    assert(N3D_RE_ApplyPickupAtCell(63, 63).kind ==
           N3D_PICKUP_UNRESOLVED);

    /*
     * The collision oracle may touch the same pickup on several one-unit
     * substeps. Accepted pickup deactivation makes subsequent touches no-op.
     */
    uint8_t collision_pickup_payload[N3D_MAP_LEVEL_BYTES] = {0};
    const int collision_pickup_cell = 8 * N3D_MAP_WIDTH + 8;
    const int collision_empty_cell = 8 * N3D_MAP_WIDTH + 9;
    collision_pickup_payload[
        collision_pickup_cell * N3D_MAP_CELL_BYTES + 1] = 0x2A;

    assert(N3D_RE_LoadMapPayload(
        collision_pickup_payload,
        sizeof(collision_pickup_payload)));

    n3d_player.laser_ammo = 90;
    n3d_pickup_touch_context pickup_touch_context = {0};
    n3d_collision_callbacks pickup_collision_callbacks = {
        N3D_RE_DoorCellPassableCallback,
        NULL,
        N3D_RE_PlayerPickupTouchCallback,
        &pickup_touch_context
    };

    n3d_resolved_collision_result pickup_collision =
        N3D_RE_TestResolvedLeadingEdgePair(
            8, 8, 9, 8, 1,
            &pickup_collision_callbacks);

    assert(pickup_collision.resolved);
    assert(pickup_collision.step == 1);
    assert(n3d_player.laser_ammo == 110);
    assert(pickup_touch_context.touch_calls == 1);
    assert(pickup_touch_context.accepted_pickups == 1);
    assert(pickup_touch_context.last_result.kind ==
           N3D_PICKUP_AMMO_ADDED);

    pickup_collision =
        N3D_RE_TestResolvedLeadingEdgePair(
            8, 8, 9, 8, 1,
            &pickup_collision_callbacks);

    assert(pickup_collision.resolved);
    assert(pickup_collision.step == 1);
    assert(n3d_player.laser_ammo == 110);
    assert(pickup_touch_context.touch_calls == 2);
    assert(pickup_touch_context.accepted_pickups == 1);
    assert(pickup_touch_context.last_result.kind ==
           N3D_PICKUP_INACTIVE);

    const int collision_pickup_slot =
        N3D_RE_FindObjectSlotByCell(8, 8);
    assert(collision_pickup_slot >= 0);
    assert((n3d_objects[collision_pickup_slot].flags &
            N3D_OBJECT_RUNTIME_PRESENT) == 0);
    (void)collision_empty_cell;

    /*
     * Remaining statically closed pickup classes can be tested directly by
     * runtime class/subtype even when no supplied MAP currently instantiates
     * that class.
     */
    N3D_RE_ResetRuntime();
    N3D_RE_ResetPlayer();

    n3d_object_count = 1;
    n3d_objects[0].flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;

    n3d_objects[0].object_class = 0x31;
    n3d_player.score = 10;
    pickup_result = N3D_RE_ApplyPickupObject(0);
    assert(pickup_result.kind == N3D_PICKUP_SCORE_ADDED);
    assert(pickup_result.score_delta == 200);
    assert(n3d_player.score == 210);
    assert(pickup_result.accepted == 1);
    assert((n3d_objects[0].flags & N3D_OBJECT_RUNTIME_PRESENT) == 0);

    memset(&n3d_objects[0], 0, sizeof(n3d_objects[0]));
    n3d_objects[0].flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;
    n3d_objects[0].object_class = 0x33;
    n3d_objects[0].variant = 0;
    n3d_player.health = 99;
    pickup_result = N3D_RE_ApplyPickupObject(0);
    assert(pickup_result.kind == N3D_PICKUP_HEALTH_ADDED);
    assert(pickup_result.value_before == 99);
    assert(pickup_result.value_after == 119);
    assert(n3d_player.health == 119);
    N3D_RE_ClampPlayerResourcesForHud();
    assert(n3d_player.health == 100);

    memset(&n3d_objects[0], 0, sizeof(n3d_objects[0]));
    n3d_objects[0].flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;
    n3d_objects[0].object_class = 0x33;
    n3d_objects[0].variant = 1;
    n3d_player.health = 80;
    pickup_result = N3D_RE_ApplyPickupObject(0);
    assert(pickup_result.kind == N3D_PICKUP_HEALTH_ADDED);
    assert(n3d_player.health == 90);

    memset(&n3d_objects[0], 0, sizeof(n3d_objects[0]));
    n3d_objects[0].flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;
    n3d_objects[0].object_class = 0x33;
    n3d_player.health = 100;
    pickup_result = N3D_RE_ApplyPickupObject(0);
    assert(pickup_result.kind == N3D_PICKUP_HEALTH_AT_THRESHOLD);
    assert(pickup_result.accepted == 0);
    assert((n3d_objects[0].flags & N3D_OBJECT_RUNTIME_PRESENT) != 0);

    memset(&n3d_objects[0], 0, sizeof(n3d_objects[0]));
    n3d_objects[0].flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;
    n3d_objects[0].object_class = 0x34;
    n3d_player.health = 99;
    n3d_player.score = 0;
    pickup_result = N3D_RE_ApplyPickupObject(0);
    assert(pickup_result.kind == N3D_PICKUP_HEALTH_ADDED);
    assert(n3d_player.health == 129);
    assert(n3d_player.score == 250);
    assert(pickup_result.score_delta == 250);
    N3D_RE_ClampPlayerResourcesForHud();
    assert(n3d_player.health == 100);

    memset(&n3d_objects[0], 0, sizeof(n3d_objects[0]));
    n3d_objects[0].flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;
    n3d_objects[0].object_class = 0x35;
    n3d_player.health = 12;
    n3d_player.laser_ammo = 9;
    n3d_player.score = 100;
    n3d_player.pickup_counter_4c1e = 4;
    pickup_result = N3D_RE_ApplyPickupObject(0);
    assert(pickup_result.kind == N3D_PICKUP_RESTORE_BONUS);
    assert(n3d_player.health == 100);
    assert(n3d_player.laser_ammo == 100);
    assert(n3d_player.score == 600);
    assert(n3d_player.pickup_counter_4c1e == 5);
    assert(pickup_result.score_delta == 500);

    for(uint8_t weapon = 0; weapon < N3D_WEAPON_COUNT; ++weapon)
    {
        memset(&n3d_objects[0], 0, sizeof(n3d_objects[0]));
        n3d_objects[0].flags =
            N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;
        n3d_objects[0].object_class = 0x36;
        n3d_objects[0].variant = weapon;

        n3d_player.owned_weapons = 0;
        n3d_player.queued_weapon = N3D_WEAPON_NONE;
        n3d_player.weapon_ui_mode = 0;
        n3d_player.silver_ammo = 1;
        n3d_player.laser_ammo = 2;
        n3d_player.wand_ammo = 3;

        pickup_result = N3D_RE_ApplyPickupObject(0);
        assert(pickup_result.kind == N3D_PICKUP_WEAPON_GRANTED);
        assert(pickup_result.accepted == 1);
        assert((n3d_player.owned_weapons & (1u << weapon)) != 0);
        assert(n3d_player.queued_weapon == weapon);
        assert(n3d_player.weapon_ui_mode ==
               (weapon == N3D_WEAPON_SILVER_PISTOL ? 1 : 2));

        if(weapon == N3D_WEAPON_MAGIC_WAND)
            assert(n3d_player.wand_ammo == N3D_WEAPON_START_AMMO);
        else if(weapon == N3D_WEAPON_SILVER_PISTOL)
            assert(n3d_player.silver_ammo == N3D_WEAPON_START_AMMO);
        else
            assert(n3d_player.laser_ammo == N3D_WEAPON_START_AMMO);
    }

    memset(&n3d_objects[0], 0, sizeof(n3d_objects[0]));
    n3d_objects[0].flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;
    n3d_objects[0].object_class = 0x38;
    n3d_player.resource_4c21 = 99;
    pickup_result = N3D_RE_ApplyPickupObject(0);
    assert(pickup_result.kind == N3D_PICKUP_RESOURCE_ADDED);
    assert(n3d_player.resource_4c21 == 119);

    memset(&n3d_objects[0], 0, sizeof(n3d_objects[0]));
    n3d_objects[0].flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;
    n3d_objects[0].object_class = 0x38;
    n3d_player.resource_4c21 = 100;
    pickup_result = N3D_RE_ApplyPickupObject(0);
    assert(pickup_result.kind == N3D_PICKUP_RESOURCE_AT_THRESHOLD);
    assert(pickup_result.accepted == 0);
    assert((n3d_objects[0].flags & N3D_OBJECT_RUNTIME_PRESENT) != 0);

    memset(&n3d_objects[0], 0, sizeof(n3d_objects[0]));
    n3d_objects[0].flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;
    n3d_objects[0].object_class = 0x3A;
    n3d_player.crystal_ball_charge = 99;
    pickup_result = N3D_RE_ApplyPickupObject(0);
    assert(pickup_result.kind == N3D_PICKUP_CRYSTAL_CHARGE_ADDED);
    assert(n3d_player.crystal_ball_charge == 119);
    N3D_RE_ClampPlayerResourcesForHud();
    assert(n3d_player.crystal_ball_charge == 100);

    memset(&n3d_objects[0], 0, sizeof(n3d_objects[0]));
    n3d_objects[0].flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;
    n3d_objects[0].object_class = 0x3B;
    n3d_player.magic_eye_charge = 99;
    pickup_result = N3D_RE_ApplyPickupObject(0);
    assert(pickup_result.kind == N3D_PICKUP_EYE_CHARGE_ADDED);
    assert(n3d_player.magic_eye_charge == 119);
    N3D_RE_ClampPlayerResourcesForHud();
    assert(n3d_player.magic_eye_charge == 100);

    memset(&n3d_objects[0], 0, sizeof(n3d_objects[0]));
    n3d_objects[0].flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;
    n3d_objects[0].object_class = 0x32;
    pickup_result = N3D_RE_ApplyPickupObject(0);
    assert(pickup_result.kind == N3D_PICKUP_DEFERRED);
    assert((n3d_objects[0].flags & N3D_OBJECT_RUNTIME_PRESENT) != 0);

    memset(&n3d_objects[0], 0, sizeof(n3d_objects[0]));
    n3d_objects[0].flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;
    n3d_objects[0].object_class = 0x37;
    pickup_result = N3D_RE_ApplyPickupObject(0);
    assert(pickup_result.kind == N3D_PICKUP_DEFERRED);

    memset(&n3d_objects[0], 0, sizeof(n3d_objects[0]));
    n3d_objects[0].flags = N3D_OBJECT_RUNTIME_PRESENT | N3D_OBJECT_SPECIAL_TOUCH;
    n3d_objects[0].object_class = 0x3D;
    pickup_result = N3D_RE_ApplyPickupObject(0);
    assert(pickup_result.kind == N3D_PICKUP_DEFERRED);

    /* Execute exact USE effects that are already closed by RE evidence. */
    uint8_t use_payload[N3D_MAP_LEVEL_BYTES] = {0};
    const int adjacent_cell = 10 * N3D_MAP_WIDTH + 11;
    const int push_destination_cell = 10 * N3D_MAP_WIDTH + 12;

    /* PANEL: OBJECTS class SECRET -> mapped object type 0x03. */
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES + 1] = 0x62;
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;

    n3d_use_execution use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_PANEL_ACTIVATED);
    assert(use_execution.event_id == N3D_PANEL_USE_EVENT);
    assert(use_execution.runtime_slot >= 0);
    assert(N3D_RE_PanelActivation(
        &n3d_panels[use_execution.runtime_slot]) == 2);

    /* PUSH: empty destination starts 8 x 8-unit movement. */
    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES + 1] = 0x18;
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;

    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_PUSH_STARTED);
    assert(use_execution.runtime_slot >= 0);
    assert(n3d_pushes[use_execution.runtime_slot].steps_remaining == 8);

    const int pushed_object_slot = N3D_RE_FindObjectSlotByCell(11, 10);
    assert(pushed_object_slot >= 0);
    const int16_t push_start_x = n3d_objects[pushed_object_slot].world_x;
    const int16_t push_start_y = n3d_objects[pushed_object_slot].world_y;

    for(int i = 0; i < 8; ++i)
    {
        int completed = 0;
        assert(N3D_RE_StepPushObject(
            use_execution.runtime_slot, &completed));
        assert(completed == (i == 7));
    }

    assert(n3d_objects[pushed_object_slot].world_x == push_start_x + 64);
    assert(n3d_objects[pushed_object_slot].world_y == push_start_y);
    assert(n3d_pushes[use_execution.runtime_slot].steps_remaining == 0);

    /* A confirmed blocking wall in the destination rejects the push. */
    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES + 1] = 0x18;
    use_payload[push_destination_cell * N3D_MAP_CELL_BYTES] = 0x01;
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_PUSH_BLOCKED);

    /* Unknown destination mapping is kept unresolved instead of guessed. */
    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES + 1] = 0x18;
    use_payload[push_destination_cell * N3D_MAP_CELL_BYTES] = 0x71;
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_UNRESOLVED);

    /* Door target is linked, but ordinary door transition remains deferred. */
    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES] = 0x70;
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_DOOR_DEFERRED);
    assert(use_execution.runtime_slot ==
           N3D_RE_FindDoorSlotByCell(11, 10));

    /* Known wall/object families without closed effects stay explicit deferred. */
    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES] = 0x90; /* WARP_1 */
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_WALL_DEFERRED);

    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES + 1] = 0x05;
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_OBJECT_DEFERRED);

    /* LEVEL_UP / LEVEL_UP2 return the exact zero-based level delta request. */
    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES] = 0x92;
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_LEVEL_CHANGE_REQUEST);
    assert(use_execution.level_delta == 1);

    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES] = 0x96;
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_LEVEL_CHANGE_REQUEST);
    assert(use_execution.level_delta == 2);

    /* WARP_L4 checks colored-key bit 3 and never consumes it here. */
    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES] = 0x91;
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;
    n3d_player.colored_keys = 0;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_KEY_GATE_BLOCKED);
    assert(use_execution.required_inventory_bit == 3);

    n3d_player.colored_keys = 0x08;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_KEY_GATE_PASSED);
    assert(use_execution.required_inventory_bit == 3);
    assert(n3d_player.colored_keys == 0x08);

    /* WARP_L1 uses bit 0. */
    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES] = 0x97;
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;
    n3d_player.colored_keys = 0x01;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_KEY_GATE_PASSED);
    assert(use_execution.required_inventory_bit == 0);

    /* WARP_S1 requires all four pentagram bits. */
    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES] = 0x95;
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;
    n3d_player.pentagrams = 0x07;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_PENTAGRAM_GATE_BLOCKED);

    n3d_player.pentagrams = 0x0F;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_PENTAGRAM_GATE_PASSED);

    /* WARP_S2 remains a known class with intentionally deferred effect. */
    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES] = 0x93;
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_WALL_DEFERRED);

    /*
     * Player movement kernel: one-unit axis attempts with 28-unit leading
     * probes, 27-unit perpendicular corners and independent-axis sliding.
     */
    N3D_RE_ResetRuntime();
    N3D_RE_InitPlayerAtTile(10, 10);

    /* Empty raw cell 0 is a known zero-property cell. */
    n3d_wall_mapping_known[0] = 1;
    n3d_wall_property_known[0] = 1;
    n3d_wall_mapped_type[0] = 0;
    n3d_wall_property_resolved[0] = 0;
    n3d_object_mapping_known[0] = 1;
    n3d_object_property_known[0] = 1;
    n3d_object_mapped_type[0] = 0;
    n3d_object_property_resolved[0] = 0;

    n3d_collision_callbacks player_move_callbacks = {
        N3D_RE_DoorCellPassableCallback,
        NULL,
        N3D_RE_PlayerPickupTouchCallback,
        NULL
    };

    const int16_t move_start_x = n3d_player.world_x;
    const int16_t move_start_y = n3d_player.world_y;

    n3d_player_move_result move_result =
        N3D_RE_MovePlayerWorldDelta(
            10, 0, &player_move_callbacks);
    assert(move_result.requested_x == 10);
    assert(move_result.requested_y == 0);
    assert(move_result.accepted_x == 10);
    assert(move_result.accepted_y == 0);
    assert(move_result.x_attempts == 10);
    assert(move_result.x_blocked == 0);
    assert(!move_result.unresolved_collision);
    assert(n3d_player.world_x == move_start_x + 10);
    assert(n3d_player.world_y == move_start_y);

    N3D_RE_InitPlayerAtTile(10, 10);
    move_result = N3D_RE_MovePlayerWorldDelta(
        10, 5, &player_move_callbacks);
    assert(move_result.accepted_x == 10);
    assert(move_result.accepted_y == 5);
    assert(move_result.x_attempts == 10);
    assert(move_result.y_attempts == 5);

    /*
     * Adjacent hard wall: X advances only until the +28 leading probe reaches
     * tile 11; Y continues, demonstrating the original independent-axis slide.
     */
    N3D_RE_InitPlayerAtTile(10, 10);
    n3d_map[10 * N3D_MAP_WIDTH + 11].wall = 1;
    n3d_wall_mapping_known[1] = 1;
    n3d_wall_property_known[1] = 1;
    n3d_wall_mapped_type[1] = 0x01;
    n3d_wall_property_resolved[1] =
        N3D_RE_WallPropertiesForMappedType(0x01);

    move_result = N3D_RE_MovePlayerWorldDelta(
        10, 10, &player_move_callbacks);
    assert(move_result.accepted_x == 3);
    assert(move_result.x_blocked == 7);
    assert(move_result.accepted_y == 10);
    assert(move_result.y_blocked == 0);
    assert(n3d_player.world_x ==
           10 * N3D_WORLD_UNITS_PER_TILE +
           N3D_TILE_CENTER_OFFSET + 3);
    assert(n3d_player.world_y ==
           10 * N3D_WORLD_UNITS_PER_TILE +
           N3D_TILE_CENTER_OFFSET + 10);

    /*
     * Forced-closed door at the player's own cell: normal center (32,32) lies
     * in the recovered 8x8 trap core, so all four cardinal +/-1 attempts block.
     */
    N3D_RE_ResetRuntime();
    N3D_RE_InitPlayerAtTile(10, 10);
    n3d_wall_mapping_known[0] = 1;
    n3d_wall_property_known[0] = 1;
    n3d_wall_mapped_type[0] = 0;
    n3d_wall_property_resolved[0] = 0;
    n3d_object_mapping_known[0] = 1;
    n3d_object_property_known[0] = 1;
    n3d_object_mapped_type[0] = 0;
    n3d_object_property_resolved[0] = 0;

    n3d_wall_mapping_known[0x70] = 1;
    n3d_wall_property_known[0x70] = 1;
    n3d_wall_mapped_type[0x70] = 0x31;
    n3d_wall_property_resolved[0x70] =
        N3D_RE_WallPropertiesForMappedType(0x31);
    n3d_map[10 * N3D_MAP_WIDTH + 10].wall = 0x70;

    const int trap_door_slot =
        N3D_RE_RegisterDoorCell(10, 10);
    assert(trap_door_slot >= 0);
    assert(N3D_RE_SetDoorCellState(10, 10, 3));

    const int16_t trap_x = n3d_player.world_x;
    const int16_t trap_y = n3d_player.world_y;

    move_result = N3D_RE_MovePlayerWorldDelta(
        1, 0, &player_move_callbacks);
    assert(move_result.accepted_x == 0);
    assert(n3d_player.world_x == trap_x);

    move_result = N3D_RE_MovePlayerWorldDelta(
        -1, 0, &player_move_callbacks);
    assert(move_result.accepted_x == 0);
    assert(n3d_player.world_x == trap_x);

    move_result = N3D_RE_MovePlayerWorldDelta(
        0, 1, &player_move_callbacks);
    assert(move_result.accepted_y == 0);
    assert(n3d_player.world_y == trap_y);

    move_result = N3D_RE_MovePlayerWorldDelta(
        0, -1, &player_move_callbacks);
    assert(move_result.accepted_y == 0);
    assert(n3d_player.world_y == trap_y);

    assert(N3D_RE_SetDoorCellState(10, 10, 0));
    move_result = N3D_RE_MovePlayerWorldDelta(
        1, 0, &player_move_callbacks);
    assert(move_result.accepted_x == 1);
    assert(n3d_player.world_x == trap_x + 1);

    /*
     * Exact E516/9D30 angle-driven movement. 0 degrees is north (Y-),
     * 90 east (X+), 180 south (Y+), 270 west (X-).
     */
    {
        uint8_t exact_trig_fixture[N3D_TRIG_FILE_BYTES] = {0};
        write_s16_le(exact_trig_fixture, 45 * 2, 724);
        write_s16_le(exact_trig_fixture, 90 * 2, 1024);
        write_s16_le(exact_trig_fixture, 180 * 2, 0);
        write_s16_le(exact_trig_fixture, 270 * 2, -1024);
        write_s16_le(
            exact_trig_fixture,
            N3D_TRIG_ANGLE_COUNT * 2 + 0 * 2,
            1024);
        write_s16_le(
            exact_trig_fixture,
            N3D_TRIG_ANGLE_COUNT * 2 + 45 * 2,
            724);
        write_s16_le(
            exact_trig_fixture,
            N3D_TRIG_ANGLE_COUNT * 2 + 180 * 2,
            -1024);
        write_s16_le(
            exact_trig_fixture,
            N3D_TRIG_ANGLE_COUNT * 2 + 270 * 2,
            0);

        FILE* exact_trig_file =
            fopen("N3D_TRIG_Q10_EXACT_MOVE_TEST.BIN", "wb");
        assert(exact_trig_file != NULL);
        assert(fwrite(
            exact_trig_fixture,
            1,
            sizeof(exact_trig_fixture),
            exact_trig_file) == sizeof(exact_trig_fixture));
        fclose(exact_trig_file);

        assert(N3D_RE_LoadTrigQ10(
            "N3D_TRIG_Q10_EXACT_MOVE_TEST.BIN"));

        N3D_RE_ResetRuntime();
        n3d_wall_mapping_known[0] = 1;
        n3d_wall_property_known[0] = 1;
        n3d_wall_mapped_type[0] = 0;
        n3d_wall_property_resolved[0] = 0;
        n3d_object_mapping_known[0] = 1;
        n3d_object_property_known[0] = 1;
        n3d_object_mapped_type[0] = 0;
        n3d_object_property_resolved[0] = 0;

        n3d_collision_callbacks exact_move_callbacks = {
            N3D_RE_DoorCellPassableCallback,
            NULL,
            N3D_RE_PlayerPickupTouchCallback,
            NULL
        };

        N3D_RE_InitPlayerAtTile(20, 20);
        const int16_t exact_start_x = n3d_player.world_x;
        const int16_t exact_start_y = n3d_player.world_y;

        n3d_player_move_result exact_move =
            N3D_RE_MovePlayerAngleSubsteps(
                0, 10, &exact_move_callbacks);
        assert(exact_move.requested_x == 0);
        assert(exact_move.requested_y == -10);
        assert(exact_move.accepted_x == 0);
        assert(exact_move.accepted_y == -10);
        assert(n3d_player.world_x == exact_start_x);
        assert(n3d_player.world_y == exact_start_y - 10);

        N3D_RE_InitPlayerAtTile(20, 20);
        exact_move = N3D_RE_MovePlayerAngleSubsteps(
            90, 10, &exact_move_callbacks);
        assert(exact_move.requested_x == 10);
        assert(exact_move.requested_y == 0);
        assert(exact_move.accepted_x == 10);
        assert(exact_move.accepted_y == 0);
        assert(n3d_player.world_x == exact_start_x + 10);
        assert(n3d_player.world_y == exact_start_y);

        N3D_RE_InitPlayerAtTile(20, 20);
        exact_move = N3D_RE_MovePlayerAngleSubsteps(
            180, 10, &exact_move_callbacks);
        assert(exact_move.requested_x == 0);
        assert(exact_move.requested_y == 10);
        assert(exact_move.accepted_y == 10);
        assert(n3d_player.world_y == exact_start_y + 10);

        N3D_RE_InitPlayerAtTile(20, 20);
        exact_move = N3D_RE_MovePlayerAngleSubsteps(
            270, 10, &exact_move_callbacks);
        assert(exact_move.requested_x == -10);
        assert(exact_move.requested_y == 0);
        assert(exact_move.accepted_x == -10);
        assert(n3d_player.world_x == exact_start_x - 10);

        N3D_RE_InitPlayerAtTile(20, 20);
        exact_move = N3D_RE_MovePlayerAngleSubsteps(
            45, 10, &exact_move_callbacks);
        assert(exact_move.requested_x == 10);
        assert(exact_move.requested_y == -10);
        assert(exact_move.accepted_x == 10);
        assert(exact_move.accepted_y == -10);
        assert(exact_move.x_attempts == 10);
        assert(exact_move.y_attempts == 10);
        assert(n3d_player.world_x == exact_start_x + 10);
        assert(n3d_player.world_y == exact_start_y - 10);

        remove("N3D_TRIG_Q10_EXACT_MOVE_TEST.BIN");
    }

    /* Recovered FIRE preflight: jam/ammo/pool ordering without guessed cadence. */
    N3D_RE_ResetRuntime();
    N3D_RE_ResetPlayer();

    n3d_fire_result fire_result =
        N3D_RE_TryBeginPlayerFire(20);
    assert(fire_result.kind == N3D_FIRE_NO_WEAPON);

    n3d_player.active_weapon = N3D_WEAPON_SINGLE_LASER;
    n3d_player.laser_ammo = 5;
    n3d_player.weapon_jam = 1;
    fire_result = N3D_RE_TryBeginPlayerFire(20);
    assert(fire_result.kind == N3D_FIRE_JAMMED);
    assert(n3d_player.laser_ammo == 5);
    assert(N3D_RE_FirstFreeProjectileSlot() == 0);

    n3d_player.weapon_jam = 0;
    n3d_player.laser_ammo = 0;
    fire_result = N3D_RE_TryBeginPlayerFire(20);
    assert(fire_result.kind == N3D_FIRE_NO_AMMO);
    assert(n3d_player.laser_ammo == 0);
    assert(N3D_RE_FirstFreeProjectileSlot() == 0);

    n3d_player.laser_ammo = 5;
    for(int i = 0; i < N3D_MAX_PROJECTILES; ++i)
        n3d_projectiles[i].state = 1;

    fire_result = N3D_RE_TryBeginPlayerFire(20);
    assert(fire_result.kind == N3D_FIRE_PROJECTILE_POOL_FULL);
    assert(n3d_player.laser_ammo == 5);
    assert(fire_result.ammo_consumed == 0);

    N3D_RE_ResetRuntime();
    N3D_RE_InitPlayerAtTile(4, 5);
    n3d_player.active_weapon = N3D_WEAPON_SINGLE_LASER;
    n3d_player.laser_ammo = 5;

    fire_result = N3D_RE_TryBeginPlayerFire(20);
    assert(fire_result.kind == N3D_FIRE_PROJECTILE_READY);
    assert(fire_result.projectile_slot == 0);
    assert(fire_result.weapon_selector == N3D_WEAPON_SINGLE_LASER);
    assert(fire_result.ammo_before == 5);
    assert(fire_result.ammo_after == 4);
    assert(fire_result.ammo_consumed == 1);
    assert(n3d_player.laser_ammo == 4);
    assert(n3d_projectiles[0].state == 1);
    assert(n3d_projectiles[0].object.world_x == n3d_player.world_x);
    assert(n3d_projectiles[0].object.world_y == n3d_player.world_y);
    assert(n3d_projectiles[0].object.sequence_id == 20);

    /* Selector 3 shares the exact same laser ammo byte. */
    n3d_player.active_weapon = N3D_WEAPON_CONTINUOUS_LASER;
    fire_result = N3D_RE_TryBeginPlayerFire(20);
    assert(fire_result.kind == N3D_FIRE_PROJECTILE_READY);
    assert(fire_result.projectile_slot == 1);
    assert(fire_result.ammo_before == 4);
    assert(fire_result.ammo_after == 3);
    assert(n3d_player.laser_ammo == 3);

    /* Silver Pistol is hitscan and must not allocate a projectile slot. */
    N3D_RE_ResetRuntime();
    N3D_RE_InitPlayerAtTile(4, 5);
    n3d_player.active_weapon = N3D_WEAPON_SILVER_PISTOL;
    n3d_player.silver_ammo = 7;
    fire_result = N3D_RE_TryBeginPlayerFire(20);
    assert(fire_result.kind == N3D_FIRE_HITSCAN_READY);
    assert(fire_result.projectile_slot == -1);
    assert(fire_result.ammo_before == 7);
    assert(fire_result.ammo_after == 6);
    assert(n3d_player.silver_ammo == 6);
    assert(N3D_RE_FirstFreeProjectileSlot() == 0);

    /* Wand uses its own ammo pool and projectile sequence +2. */
    n3d_player.active_weapon = N3D_WEAPON_MAGIC_WAND;
    n3d_player.wand_ammo = 3;
    fire_result = N3D_RE_TryBeginPlayerFire(20);
    assert(fire_result.kind == N3D_FIRE_PROJECTILE_READY);
    assert(fire_result.projectile_slot == 0);
    assert(fire_result.ammo_before == 3);
    assert(fire_result.ammo_after == 2);
    assert(n3d_projectiles[0].object.sequence_id == 22);

    /* Omnipotent permits firing with zero ammo and does not decrement it. */
    N3D_RE_ResetRuntime();
    N3D_RE_InitPlayerAtTile(4, 5);
    n3d_player.active_weapon = N3D_WEAPON_SINGLE_LASER;
    n3d_player.laser_ammo = 0;
    n3d_player.omnipotent = 1;
    fire_result = N3D_RE_TryBeginPlayerFire(20);
    assert(fire_result.kind == N3D_FIRE_PROJECTILE_READY);
    assert(fire_result.ammo_before == 0);
    assert(fire_result.ammo_after == 0);
    assert(fire_result.ammo_consumed == 0);
    assert(n3d_player.laser_ammo == 0);

    /* Projectile line state shares exact E516/9D30 integer stepping. */
    {
        uint8_t projectile_trig_fixture[N3D_TRIG_FILE_BYTES] = {0};
        write_s16_le(projectile_trig_fixture, 45 * 2, 724);
        write_s16_le(projectile_trig_fixture, 90 * 2, 1024);
        write_s16_le(projectile_trig_fixture, 180 * 2, 0);
        write_s16_le(projectile_trig_fixture, 270 * 2, -1024);
        write_s16_le(
            projectile_trig_fixture,
            N3D_TRIG_ANGLE_COUNT * 2 + 0 * 2,
            1024);
        write_s16_le(
            projectile_trig_fixture,
            N3D_TRIG_ANGLE_COUNT * 2 + 45 * 2,
            724);
        write_s16_le(
            projectile_trig_fixture,
            N3D_TRIG_ANGLE_COUNT * 2 + 90 * 2,
            0);
        write_s16_le(
            projectile_trig_fixture,
            N3D_TRIG_ANGLE_COUNT * 2 + 180 * 2,
            -1024);
        write_s16_le(
            projectile_trig_fixture,
            N3D_TRIG_ANGLE_COUNT * 2 + 270 * 2,
            0);

        FILE* projectile_trig_file =
            fopen("N3D_TRIG_Q10_PROJECTILE_TEST.BIN", "wb");
        assert(projectile_trig_file != NULL);
        assert(fwrite(
            projectile_trig_fixture,
            1,
            sizeof(projectile_trig_fixture),
            projectile_trig_file) == sizeof(projectile_trig_fixture));
        fclose(projectile_trig_file);

        assert(N3D_RE_LoadTrigQ10(
            "N3D_TRIG_Q10_PROJECTILE_TEST.BIN"));

        N3D_RE_ResetRuntime();
        assert(N3D_RE_InitializeProjectileFromAngle(
            0, N3D_WEAPON_SINGLE_LASER, 100, 200, 20, 0));
        assert(n3d_projectiles[0].x_is_major_axis == 0);
        assert(n3d_projectiles[0].step_y == -1);
        assert(N3D_RE_AdvanceProjectileLineSteps(0, 10) == 10);
        assert(n3d_projectiles[0].object.world_x == 100);
        assert(n3d_projectiles[0].object.world_y == 190);

        N3D_RE_ResetRuntime();
        assert(N3D_RE_InitializeProjectileFromAngle(
            0, N3D_WEAPON_SINGLE_LASER, 100, 200, 20, 90));
        assert(n3d_projectiles[0].x_is_major_axis == 1);
        assert(n3d_projectiles[0].step_x == 1);
        assert(N3D_RE_AdvanceProjectileLineSteps(0, 10) == 10);
        assert(n3d_projectiles[0].object.world_x == 110);
        assert(n3d_projectiles[0].object.world_y == 200);

        N3D_RE_ResetRuntime();
        assert(N3D_RE_InitializeProjectileFromAngle(
            0, N3D_WEAPON_MAGIC_WAND, 100, 200, 20, 45));
        assert(n3d_projectiles[0].x_is_major_axis == 0);
        assert(n3d_projectiles[0].step_x == 1);
        assert(n3d_projectiles[0].step_y == -1);
        assert(N3D_RE_AdvanceProjectileLineSteps(0, 10) == 10);
        assert(n3d_projectiles[0].object.world_x == 110);
        assert(n3d_projectiles[0].object.world_y == 190);

        remove("N3D_TRIG_Q10_PROJECTILE_TEST.BIN");
    }

    /* Scripted weapon jam events are exact and deterministic. */
    N3D_RE_ResetPlayer();
    n3d_weapon_jam_event_result jam_event =
        N3D_RE_ApplyWeaponJamEvent(N3D_WEAPON_JAM_ENABLE_EVENT);
    assert(jam_event.handled == 1);
    assert(jam_event.jammed == 1);
    assert(jam_event.sound_id == N3D_WEAPON_JAM_SOUND_ID);
    assert(n3d_player.weapon_jam == 1);

    jam_event = N3D_RE_ApplyWeaponJamEvent(0x46);
    assert(jam_event.handled == 0);
    assert(jam_event.jammed == 1);
    assert(jam_event.sound_id == 0);
    assert(n3d_player.weapon_jam == 1);

    jam_event =
        N3D_RE_ApplyWeaponJamEvent(N3D_WEAPON_JAM_DISABLE_EVENT);
    assert(jam_event.handled == 1);
    assert(jam_event.jammed == 0);
    assert(jam_event.sound_id == N3D_WEAPON_JAM_SOUND_ID);
    assert(n3d_player.weapon_jam == 0);

    /* Projectile collision classification mirrors the closed 9B64 branches. */
    {
        N3D_RE_ResetRuntime();

        /* Ensure empty raw cell 0 remains fully resolved and non-colliding. */
        n3d_wall_mapping_known[0] = 1;
        n3d_wall_property_known[0] = 1;
        n3d_wall_mapped_type[0] = 0;
        n3d_wall_property_resolved[0] = 0;
        n3d_object_mapping_known[0] = 1;
        n3d_object_property_known[0] = 1;
        n3d_object_mapped_type[0] = 0;
        n3d_object_property_resolved[0] = 0;

        assert(N3D_RE_InitializeProjectile(
            0, N3D_WEAPON_SINGLE_LASER, 10 * 64 + 32, 10 * 64 + 32, 20));

        n3d_projectile_collision_result projectile_collision =
            N3D_RE_ClassifyProjectileCollision(0);
        assert(projectile_collision.kind == N3D_PROJECTILE_COLLISION_NONE);
        assert(projectile_collision.enter_impact == 0);

        /* Unknown raw wall mapping must remain explicitly unresolved. */
        n3d_map[10 * N3D_MAP_WIDTH + 10].wall = 0x71;
        projectile_collision = N3D_RE_ClassifyProjectileCollision(0);
        assert(projectile_collision.kind ==
               N3D_PROJECTILE_COLLISION_UNRESOLVED);

        /*
         * GUARD cell path: object property bit 0x08 gates the lookup and the
         * exact +/-9 world tolerance decides the hit.
         */
        n3d_map[10 * N3D_MAP_WIDTH + 10].wall = 0;
        n3d_map[10 * N3D_MAP_WIDTH + 10].object = 0x80;

        n3d_object_mapping_known[0x80] = 1;
        n3d_object_property_known[0x80] = 1;
        n3d_object_mapped_type[0x80] = 0x08;
        n3d_object_property_resolved[0x80] =
            N3D_RE_ObjectPropertiesForMappedType(0x08);

        int guard_object_slot = -1;
        assert(N3D_RE_InstantiateMapObject(
            0x80, 10, 10, &guard_object_slot));
        assert(guard_object_slot >= 0);
        assert(n3d_guard_count == 1);

        n3d_projectiles[0].object.world_x =
            n3d_objects[guard_object_slot].world_x + 9;
        n3d_projectiles[0].object.world_y =
            n3d_objects[guard_object_slot].world_y - 9;

        projectile_collision = N3D_RE_ClassifyProjectileCollision(0);
        assert(projectile_collision.kind ==
               N3D_PROJECTILE_COLLISION_GUARD_HIT);
        assert(projectile_collision.object_slot == guard_object_slot);
        assert(projectile_collision.guard_slot ==
               n3d_objects[guard_object_slot].guard_index);
        assert(projectile_collision.enter_impact == 1);

        n3d_projectiles[0].object.world_x =
            n3d_objects[guard_object_slot].world_x + 10;
        n3d_projectiles[0].object.world_y =
            n3d_objects[guard_object_slot].world_y;
        projectile_collision = N3D_RE_ClassifyProjectileCollision(0);
        assert(projectile_collision.kind == N3D_PROJECTILE_COLLISION_NONE);

        /* Explodable wall: property 0x10 requests event 0x29 and impact. */
        n3d_map[10 * N3D_MAP_WIDTH + 10].object = 0;
        n3d_map[10 * N3D_MAP_WIDTH + 10].wall = 0xFE;
        n3d_wall_mapping_known[0xFE] = 1;
        n3d_wall_property_known[0xFE] = 1;
        n3d_wall_mapped_type[0xFE] = 0x2E;
        n3d_wall_property_resolved[0xFE] =
            N3D_RE_WallPropertiesForMappedType(0x2E);

        n3d_projectiles[0].object.world_x = 10 * 64 + 32;
        n3d_projectiles[0].object.world_y = 10 * 64 + 32;
        projectile_collision = N3D_RE_ClassifyProjectileCollision(0);
        assert(projectile_collision.kind ==
               N3D_PROJECTILE_COLLISION_EXPLODABLE_WALL);
        assert(projectile_collision.mapped_wall_type == 0x2E);
        assert(projectile_collision.event_id == N3D_EXPLODABLE_WALL_EVENT);
        assert(projectile_collision.enter_impact == 1);

        /*
         * Ordinary known blocking wall is classified but its remaining 9B64
         * state/cleanup effects stay deferred.
         */
        n3d_map[10 * N3D_MAP_WIDTH + 10].wall = 0x01;
        n3d_wall_mapping_known[0x01] = 1;
        n3d_wall_property_known[0x01] = 1;
        n3d_wall_mapped_type[0x01] = 0x01;
        n3d_wall_property_resolved[0x01] =
            N3D_RE_WallPropertiesForMappedType(0x01);

        projectile_collision = N3D_RE_ClassifyProjectileCollision(0);
        assert(projectile_collision.kind ==
               N3D_PROJECTILE_COLLISION_WALL_DEFERRED);
        assert(projectile_collision.enter_impact == 0);
    }

    /* 9D30 traversal driver: move one unit, classify, stop on first collision. */
    {
        N3D_RE_ResetRuntime();

        n3d_wall_mapping_known[0] = 1;
        n3d_wall_property_known[0] = 1;
        n3d_wall_mapped_type[0] = 0;
        n3d_wall_property_resolved[0] = 0;
        n3d_object_mapping_known[0] = 1;
        n3d_object_property_known[0] = 1;
        n3d_object_mapped_type[0] = 0;
        n3d_object_property_resolved[0] = 0;

        assert(N3D_RE_InitializeProjectile(
            0, N3D_WEAPON_SINGLE_LASER,
            10 * 64 + 32, 10 * 64 + 32, 20));

        n3d_projectiles[0].x_is_major_axis = 1;
        n3d_projectiles[0].line_error = -1;
        n3d_projectiles[0].minor_error_step = 0;
        n3d_projectiles[0].major_error_fixup = 0;
        n3d_projectiles[0].step_x = 1;
        n3d_projectiles[0].step_y = -1;

        n3d_projectile_advance_result advance =
            N3D_RE_AdvanceProjectileUntilCollision(0, 5);
        assert(advance.requested_substeps == 5);
        assert(advance.advanced_substeps == 5);
        assert(advance.collision.kind ==
               N3D_PROJECTILE_COLLISION_NONE);
        assert(n3d_projectiles[0].object.world_x ==
               10 * 64 + 32 + 5);

        /* Enter a normal blocking wall on the first step. */
        N3D_RE_ResetRuntime();
        n3d_wall_mapping_known[0] = 1;
        n3d_wall_property_known[0] = 1;
        n3d_wall_mapped_type[0] = 0;
        n3d_wall_property_resolved[0] = 0;
        n3d_object_mapping_known[0] = 1;
        n3d_object_property_known[0] = 1;
        n3d_object_mapped_type[0] = 0;
        n3d_object_property_resolved[0] = 0;

        n3d_wall_mapping_known[1] = 1;
        n3d_wall_property_known[1] = 1;
        n3d_wall_mapped_type[1] = 0x01;
        n3d_wall_property_resolved[1] =
            N3D_RE_WallPropertiesForMappedType(0x01);
        n3d_map[10 * N3D_MAP_WIDTH + 11].wall = 1;

        assert(N3D_RE_InitializeProjectile(
            0, N3D_WEAPON_SINGLE_LASER,
            11 * 64 - 1, 10 * 64 + 32, 20));
        n3d_projectiles[0].x_is_major_axis = 1;
        n3d_projectiles[0].line_error = -1;
        n3d_projectiles[0].step_x = 1;
        n3d_projectiles[0].step_y = -1;

        advance = N3D_RE_AdvanceProjectileUntilCollision(0, 8);
        assert(advance.advanced_substeps == 1);
        assert(advance.collision.kind ==
               N3D_PROJECTILE_COLLISION_WALL_DEFERRED);
        assert(n3d_projectiles[0].object.world_x == 11 * 64);

        /* Unknown cell mapping also stops traversal immediately. */
        N3D_RE_ResetRuntime();
        n3d_wall_mapping_known[0] = 1;
        n3d_wall_property_known[0] = 1;
        n3d_wall_mapped_type[0] = 0;
        n3d_wall_property_resolved[0] = 0;
        n3d_object_mapping_known[0] = 1;
        n3d_object_property_known[0] = 1;
        n3d_object_mapped_type[0] = 0;
        n3d_object_property_resolved[0] = 0;
        n3d_map[10 * N3D_MAP_WIDTH + 11].wall = 0x71;

        assert(N3D_RE_InitializeProjectile(
            0, N3D_WEAPON_SINGLE_LASER,
            11 * 64 - 1, 10 * 64 + 32, 20));
        n3d_projectiles[0].x_is_major_axis = 1;
        n3d_projectiles[0].line_error = -1;
        n3d_projectiles[0].step_x = 1;
        n3d_projectiles[0].step_y = -1;

        advance = N3D_RE_AdvanceProjectileUntilCollision(0, 8);
        assert(advance.advanced_substeps == 1);
        assert(advance.collision.kind ==
               N3D_PROJECTILE_COLLISION_UNRESOLVED);

        /* Explodable wall produces exact event 0x29 on the first entered step. */
        N3D_RE_ResetRuntime();
        n3d_wall_mapping_known[0] = 1;
        n3d_wall_property_known[0] = 1;
        n3d_wall_mapped_type[0] = 0;
        n3d_wall_property_resolved[0] = 0;
        n3d_object_mapping_known[0] = 1;
        n3d_object_property_known[0] = 1;
        n3d_object_mapped_type[0] = 0;
        n3d_object_property_resolved[0] = 0;

        n3d_wall_mapping_known[0xFE] = 1;
        n3d_wall_property_known[0xFE] = 1;
        n3d_wall_mapped_type[0xFE] = 0x2E;
        n3d_wall_property_resolved[0xFE] =
            N3D_RE_WallPropertiesForMappedType(0x2E);
        n3d_map[10 * N3D_MAP_WIDTH + 11].wall = 0xFE;

        assert(N3D_RE_InitializeProjectile(
            0, N3D_WEAPON_SINGLE_LASER,
            11 * 64 - 1, 10 * 64 + 32, 20));
        n3d_projectiles[0].x_is_major_axis = 1;
        n3d_projectiles[0].line_error = -1;
        n3d_projectiles[0].step_x = 1;
        n3d_projectiles[0].step_y = -1;

        advance = N3D_RE_AdvanceProjectileUntilCollision(0, 8);
        assert(advance.advanced_substeps == 1);
        assert(advance.collision.kind ==
               N3D_PROJECTILE_COLLISION_EXPLODABLE_WALL);
        assert(advance.collision.event_id == 0x29);
        assert(advance.collision.enter_impact == 1);

        /* GUARD proximity hit stops on the first post-step 9B64 test. */
        N3D_RE_ResetRuntime();
        n3d_wall_mapping_known[0] = 1;
        n3d_wall_property_known[0] = 1;
        n3d_wall_mapped_type[0] = 0;
        n3d_wall_property_resolved[0] = 0;
        n3d_object_mapping_known[0] = 1;
        n3d_object_property_known[0] = 1;
        n3d_object_mapped_type[0] = 0;
        n3d_object_property_resolved[0] = 0;

        n3d_object_mapping_known[0x80] = 1;
        n3d_object_property_known[0x80] = 1;
        n3d_object_mapped_type[0x80] = 0x08;
        n3d_object_property_resolved[0x80] =
            N3D_RE_ObjectPropertiesForMappedType(0x08);
        n3d_map[10 * N3D_MAP_WIDTH + 11].object = 0x80;

        int traversal_guard_object = -1;
        assert(N3D_RE_InstantiateMapObject(
            0x80, 11, 10, &traversal_guard_object));
        assert(traversal_guard_object >= 0);

        assert(N3D_RE_InitializeProjectile(
            0, N3D_WEAPON_SINGLE_LASER,
            n3d_objects[traversal_guard_object].world_x - 10,
            n3d_objects[traversal_guard_object].world_y,
            20));
        n3d_projectiles[0].x_is_major_axis = 1;
        n3d_projectiles[0].line_error = -1;
        n3d_projectiles[0].step_x = 1;
        n3d_projectiles[0].step_y = -1;

        advance = N3D_RE_AdvanceProjectileUntilCollision(0, 8);
        assert(advance.advanced_substeps == 1);
        assert(advance.collision.kind ==
               N3D_PROJECTILE_COLLISION_GUARD_HIT);
        assert(advance.collision.object_slot == traversal_guard_object);
        assert(advance.collision.guard_slot ==
               n3d_objects[traversal_guard_object].guard_index);
        assert(advance.collision.enter_impact == 1);
    }

    /* Projectile -> GUARD damage bridge: explicit render/RNG inputs only. */
    {
        N3D_RE_ResetRuntime();
        N3D_RE_ResetPlayer();

        /* Map object 148 is class 0x0C (Mrs H.) in the recovered guard blocks. */
        n3d_object_mapping_known[148] = 1;
        n3d_object_property_known[148] = 1;
        n3d_object_mapped_type[148] = 0x0C;
        n3d_object_property_resolved[148] =
            N3D_RE_ObjectPropertiesForMappedType(0x0C);

        int damage_object_slot = -1;
        assert(N3D_RE_InstantiateMapObject(
            148, 8, 8, &damage_object_slot));
        assert(damage_object_slot >= 0);

        const int damage_guard_slot =
            n3d_objects[damage_object_slot].guard_index;
        assert(damage_guard_slot >= 0);
        assert(damage_guard_slot < (int)n3d_guard_count);

        n3d_objects[damage_object_slot].projected_y_base = 90;
        n3d_guards[damage_guard_slot].strength = 255;

        n3d_player.active_weapon = N3D_WEAPON_SINGLE_LASER;
        n3d_player.difficulty = 1;

        assert(N3D_RE_InitializeProjectile(
            0, N3D_WEAPON_SINGLE_LASER, 100, 100, 20));

        n3d_projectile_collision_result hit = {0};
        hit.kind = N3D_PROJECTILE_COLLISION_GUARD_HIT;
        hit.object_slot = (int16_t)damage_object_slot;
        hit.guard_slot = (int16_t)damage_guard_slot;
        hit.enter_impact = 1;

        n3d_projectile_guard_resolution guard_resolution =
            N3D_RE_ResolveProjectileGuardHit(
                0, &hit, 80, 0, 0);

        assert(guard_resolution.resolved == 1);
        assert(guard_resolution.applied_damage == 10);
        assert(guard_resolution.guard_result == N3D_GUARD_HIT_PAIN);
        assert(guard_resolution.score_delta == 0);
        assert(guard_resolution.entered_impact == 1);
        assert(n3d_guards[damage_guard_slot].strength == 245);
        assert(n3d_projectiles[0].state == 2);
        assert(n3d_projectiles[0].object.sequence_id == 21);
        assert((n3d_projectiles[0].object.flags & 0x10) != 0);

        /* Same 10 damage becomes lethal and returns the class score delta. */
        N3D_RE_ResetRuntime();
        n3d_object_mapping_known[148] = 1;
        n3d_object_property_known[148] = 1;
        n3d_object_mapped_type[148] = 0x0C;
        n3d_object_property_resolved[148] =
            N3D_RE_ObjectPropertiesForMappedType(0x0C);

        assert(N3D_RE_InstantiateMapObject(
            148, 8, 8, &damage_object_slot));
        const int lethal_guard_slot =
            n3d_objects[damage_object_slot].guard_index;
        n3d_objects[damage_object_slot].projected_y_base = 90;
        n3d_guards[lethal_guard_slot].strength = 10;

        n3d_player.active_weapon = N3D_WEAPON_SINGLE_LASER;
        n3d_player.difficulty = 1;
        assert(N3D_RE_InitializeProjectile(
            0, N3D_WEAPON_SINGLE_LASER, 100, 100, 20));

        hit.object_slot = (int16_t)damage_object_slot;
        hit.guard_slot = (int16_t)lethal_guard_slot;

        guard_resolution =
            N3D_RE_ResolveProjectileGuardHit(
                0, &hit, 80, 0, 0);
        assert(guard_resolution.guard_result == N3D_GUARD_HIT_KILLED);
        assert(guard_resolution.applied_damage == 10);
        assert(guard_resolution.score_delta == 250);
        assert(n3d_guards[lethal_guard_slot].strength == 0);
        assert(n3d_projectiles[0].state == 2);

        /* Dracula lethal phase 1 transforms instead of awarding kill score. */
        N3D_RE_ResetRuntime();
        n3d_object_mapping_known[176] = 1;
        n3d_object_property_known[176] = 1;
        n3d_object_mapped_type[176] = 0x11;
        n3d_object_property_resolved[176] =
            N3D_RE_ObjectPropertiesForMappedType(0x11);

        int dracula_object_slot = -1;
        assert(N3D_RE_InstantiateMapObject(
            176, 8, 8, &dracula_object_slot));
        const int dracula_guard_slot =
            n3d_objects[dracula_object_slot].guard_index;

        n3d_objects[dracula_object_slot].projected_y_base = 112;
        n3d_guards[dracula_guard_slot].strength = 32;

        n3d_player.active_weapon = N3D_WEAPON_SINGLE_LASER;
        n3d_player.difficulty = 1;
        assert(N3D_RE_InitializeProjectile(
            0, N3D_WEAPON_SINGLE_LASER, 100, 100, 20));

        hit.object_slot = (int16_t)dracula_object_slot;
        hit.guard_slot = (int16_t)dracula_guard_slot;

        guard_resolution =
            N3D_RE_ResolveProjectileGuardHit(
                0, &hit, 80, 0, 0);
        assert(guard_resolution.guard_result ==
               N3D_GUARD_HIT_DRACULA_TRANSFORMED);
        assert(guard_resolution.applied_damage == 32);
        assert(guard_resolution.score_delta == 0);
        assert(n3d_objects[dracula_object_slot].object_class == 0x14);
        assert(n3d_guards[dracula_guard_slot].strength == 0xFF);
        assert(n3d_guards[dracula_guard_slot].state == N3D_GUARD_STATE_08);
        assert(n3d_guards[dracula_guard_slot].next_state == N3D_GUARD_STATE_02);
        assert(n3d_guards[dracula_guard_slot].timer == 1);
        assert(n3d_projectiles[0].state == 2);

        /* Negative signed producer output is ignored, never uint8-wrapped. */
        N3D_RE_ResetRuntime();
        n3d_object_mapping_known[148] = 1;
        n3d_object_property_known[148] = 1;
        n3d_object_mapped_type[148] = 0x0C;
        n3d_object_property_resolved[148] =
            N3D_RE_ObjectPropertiesForMappedType(0x0C);

        assert(N3D_RE_InstantiateMapObject(
            148, 8, 8, &damage_object_slot));
        const int negative_guard_slot =
            n3d_objects[damage_object_slot].guard_index;
        n3d_objects[damage_object_slot].projected_y_base = 70;
        n3d_guards[negative_guard_slot].strength = 255;

        n3d_player.active_weapon = N3D_WEAPON_SINGLE_LASER;
        n3d_player.difficulty = 1;
        assert(N3D_RE_InitializeProjectile(
            0, N3D_WEAPON_SINGLE_LASER, 100, 100, 20));

        hit.object_slot = (int16_t)damage_object_slot;
        hit.guard_slot = (int16_t)negative_guard_slot;

        guard_resolution =
            N3D_RE_ResolveProjectileGuardHit(
                0, &hit, 80, 0, 0);
        assert(guard_resolution.resolved == 1);
        assert(guard_resolution.applied_damage == 0);
        assert(guard_resolution.guard_result == N3D_GUARD_HIT_NO_DAMAGE);
        assert(n3d_guards[negative_guard_slot].strength == 255);
        assert(n3d_projectiles[0].state == 2);
    }

    /* Explodable-wall resolution applies only the closed 9B64 effects. */
    {
        N3D_RE_ResetRuntime();

        assert(N3D_RE_InitializeProjectile(
            0, N3D_WEAPON_SINGLE_LASER, 100, 100, 20));

        n3d_projectile_collision_result wall_hit = {0};
        wall_hit.kind = N3D_PROJECTILE_COLLISION_EXPLODABLE_WALL;
        wall_hit.event_id = N3D_EXPLODABLE_WALL_EVENT;
        wall_hit.enter_impact = 1;

        n3d_projectile_wall_resolution wall_resolution =
            N3D_RE_ResolveProjectileWallCollision(0, &wall_hit);

        assert(wall_resolution.resolved == 1);
        assert(wall_resolution.deferred == 0);
        assert(wall_resolution.event_id == N3D_EXPLODABLE_WALL_EVENT);
        assert(wall_resolution.requested_runtime_wall_class ==
               N3D_EXPLODABLE_WALL_RUNTIME_CLASS);
        assert(wall_resolution.entered_impact == 1);
        assert(n3d_projectiles[0].state == 2);
        assert(n3d_projectiles[0].object.sequence_id == 21);
        assert((n3d_projectiles[0].object.flags & 0x10) != 0);

        N3D_RE_ResetRuntime();
        assert(N3D_RE_InitializeProjectile(
            0, N3D_WEAPON_SINGLE_LASER, 100, 100, 20));

        wall_hit.kind = N3D_PROJECTILE_COLLISION_WALL_DEFERRED;
        wall_hit.event_id = 0;
        wall_hit.enter_impact = 0;

        wall_resolution =
            N3D_RE_ResolveProjectileWallCollision(0, &wall_hit);

        assert(wall_resolution.resolved == 0);
        assert(wall_resolution.deferred == 1);
        assert(wall_resolution.event_id == 0);
        assert(wall_resolution.requested_runtime_wall_class == 0);
        assert(wall_resolution.entered_impact == 0);
        assert(n3d_projectiles[0].state == 1);
    }

    /* Verified USER.SAV runtime blocks: exact byte transport only. */
    {
        static uint8_t save_slot[N3D_USER_SAVE_SLOT_BYTES];
        static uint8_t rewritten_slot[N3D_USER_SAVE_SLOT_BYTES];
        static n3d_known_save_blocks save_blocks;

        memset(save_slot, 0xA5, sizeof(save_slot));
        memset(rewritten_slot, 0x5A, sizeof(rewritten_slot));

        for(size_t i = N3D_SAVE_OBJECT_OFFSET;
            i < N3D_SAVE_PUSH_OFFSET + N3D_SAVE_PUSH_BYTES;
            ++i)
        {
            save_slot[i] = (uint8_t)((i * 37u + 11u) & 0xFFu);
        }

        for(size_t i = N3D_SAVE_GUARD_WAKE_OFFSET;
            i < N3D_SAVE_GUARD_WAKE_OFFSET + N3D_SAVE_GUARD_WAKE_BYTES;
            ++i)
        {
            save_slot[i] = (uint8_t)((i * 13u + 7u) & 0xFFu);
        }

        /* Independent field fixture for the first OBJECT. */
        {
            uint8_t* p = save_slot + N3D_SAVE_OBJECT_OFFSET;
            p[0x00] = 0xAA;
            p[0x01] = 0xBB;
            p[0x02] = 0x80;
            p[0x03] = 0xFF;
            p[0x04] = 0xCC;
            p[0x05] = 0xDD;
            p[0x06] = 0xEE;
            p[0x07] = 0x12;
            p[0x08] = 0x78;
            p[0x09] = 0x56;
            p[0x0A] = 0x34;
            p[0x0B] = 0x12;
            p[0x0C] = 0x44;
            p[0x0D] = 0x33;
            p[0x0E] = 0x22;
            p[0x0F] = 0x11;
            p[0x10] = 0x00;
            p[0x11] = 0x80;
            p[0x12] = 0xFF;
            p[0x13] = 0x7F;
            p[0x18] = 0x34;
            p[0x19] = 0x12;
            p[0x1A] = 0x05;
            p[0x1B] = 0xFE;
        }

        /* Independent first GUARD fixture. */
        {
            uint8_t* p = save_slot + N3D_SAVE_GUARD_OFFSET;
            p[0x00] = 0x34;
            p[0x01] = 0x12;
            p[0x02] = 0x78;
            p[0x03] = 0x56;
            p[0x04] = 0x34;
            p[0x05] = 0x12;
            p[0x06] = 0xFF;
            p[0x07] = 0x7F;
            p[0x08] = 0x02;
            p[0x09] = 0x01;
            p[0x0A] = 3;
            p[0x0B] = N3D_GUARD_STATE_PAIN;
            p[0x0C] = N3D_GUARD_STATE_07;
            p[0x10] = 0xF5;
            p[0x13] = 0xF8;
            p[0x14] = 0x08;
        }

        /* Independent first projectile fixture. */
        {
            uint8_t* p = save_slot + N3D_SAVE_PROJECTILE_OFFSET;
            p[0x00] = 0x01;
            p[0x01] = 0x00;
            p[0x02] = 0xFE;
            p[0x03] = 0xFF;
            p[0x08] = 0x01;
            p[0x09] = 0x00;
            p[0x0A] = 0xFF;
            p[0x0B] = 0xFF;
            p[0x0C] = 2;
            p[0x0D] = 0x5A;
            p[0x0E + 0x10] = 0x34;
            p[0x0E + 0x11] = 0x12;
            p[0x0E + 0x12] = 0x78;
            p[0x0E + 0x13] = 0x56;
            p[0x0E + 0x1A] = 20;
        }

        /* Independent first PUSH fixture. */
        {
            uint8_t* p = save_slot + N3D_SAVE_PUSH_OFFSET;
            p[0x00] = 0x34;
            p[0x01] = 0x12;
            p[0x02] = 0xF8;
            p[0x03] = 0x08;
            p[0x04] = 8;
            p[0x05] = 0x7A;
        }

        save_slot[N3D_SAVE_PANEL_ACTIVATION_OFFSET] = 2;
        save_slot[N3D_SAVE_PANEL_ACTIVATION_OFFSET + 31] = 4;
        save_slot[N3D_SAVE_GLOBAL_51A4_OFFSET] = 0x91;
        save_slot[N3D_SAVE_GLOBAL_51A4_OFFSET + 7] = 0xA7;
        save_slot[N3D_SAVE_GUARD_WAKE_OFFSET] = 0x11;
        save_slot[N3D_SAVE_GUARD_WAKE_OFFSET + 63] = 0x22;

        assert(!N3D_RE_ReadKnownSaveBlocks(
            save_slot,
            N3D_USER_SAVE_SLOT_BYTES - 1,
            &save_blocks));

        assert(N3D_RE_ReadKnownSaveBlocks(
            save_slot,
            sizeof(save_slot),
            &save_blocks));

        assert(save_blocks.objects[0].map_object_id == 0xAA);
        assert(save_blocks.objects[0].variant == 0xBB);
        assert(save_blocks.objects[0].animation_aux == (int8_t)0x80);
        assert(save_blocks.objects[0].animation_frame == -1);
        assert(save_blocks.objects[0].sequence_id == 0xCC);
        assert(save_blocks.objects[0].flags == 0xDD);
        assert(save_blocks.objects[0].object_class == 0xEE);
        assert(save_blocks.objects[0].guard_index == 0x12);
        assert(save_blocks.objects[0].animation_deadline == 0x12345678u);
        assert(save_blocks.objects[0].map_cell_binding == 0x11223344u);
        assert(save_blocks.objects[0].world_x == INT16_MIN);
        assert(save_blocks.objects[0].world_y == INT16_MAX);
        assert(save_blocks.objects[0].projected_y_base == 0x1234);
        assert(save_blocks.objects[0].runtime_1a == 5);
        assert(save_blocks.objects[0].unknown_1b == 0xFE);

        assert(save_blocks.guards[0].definition_value == 0x1234);
        assert(save_blocks.guards[0].timestamp == 0x12345678u);
        assert(save_blocks.guards[0].timer == INT16_MAX);
        assert(save_blocks.guards[0].object_slot == 0x0102);
        assert(save_blocks.guards[0].strategy == 3);
        assert(save_blocks.guards[0].state == N3D_GUARD_STATE_PAIN);
        assert(save_blocks.guards[0].next_state == N3D_GUARD_STATE_07);
        assert(save_blocks.guards[0].strength == 0xF5);
        assert(save_blocks.guards[0].move_x == -8);
        assert(save_blocks.guards[0].move_y == 8);

        assert(save_blocks.projectiles[0].x_is_major_axis == 1);
        assert(save_blocks.projectiles[0].line_error == -2);
        assert(save_blocks.projectiles[0].step_x == 1);
        assert(save_blocks.projectiles[0].step_y == -1);
        assert(save_blocks.projectiles[0].state == 2);
        assert(save_blocks.projectiles[0].unknown_0d == 0x5A);
        assert(save_blocks.projectiles[0].object.world_x == 0x1234);
        assert(save_blocks.projectiles[0].object.world_y == 0x5678);
        assert(save_blocks.projectiles[0].object.runtime_1a == 20);

        assert(save_blocks.pushes[0].object_index == 0x1234);
        assert(save_blocks.pushes[0].delta_x == -8);
        assert(save_blocks.pushes[0].delta_y == 8);
        assert(save_blocks.pushes[0].steps_remaining == 8);
        assert(save_blocks.pushes[0].runtime_05 == 0x7A);

        assert(save_blocks.panel_activation[0] == 2);
        assert(save_blocks.panel_activation[31] == 4);
        assert(save_blocks.global_51a4[0] == 0x91);
        assert(save_blocks.global_51a4[7] == 0xA7);
        assert(save_blocks.guard_wake_cache[0] == 0x11);
        assert(save_blocks.guard_wake_cache[63] == 0x22);

        assert(!N3D_RE_WriteKnownSaveBlocks(
            rewritten_slot,
            N3D_USER_SAVE_SLOT_BYTES - 1,
            &save_blocks));

        assert(N3D_RE_WriteKnownSaveBlocks(
            rewritten_slot,
            sizeof(rewritten_slot),
            &save_blocks));

        for(size_t i = 0; i < sizeof(rewritten_slot); ++i)
        {
            const int known =
                (i >= N3D_SAVE_OBJECT_OFFSET &&
                 i < N3D_SAVE_PUSH_OFFSET + N3D_SAVE_PUSH_BYTES) ||
                (i >= N3D_SAVE_GUARD_WAKE_OFFSET &&
                 i < N3D_SAVE_GUARD_WAKE_OFFSET + N3D_SAVE_GUARD_WAKE_BYTES);

            if(known)
                assert(rewritten_slot[i] == save_slot[i]);
            else
                assert(rewritten_slot[i] == 0x5A);
        }
    }

    puts("C-rewrite recovered runtime self-test: PASS");
    return 0;
}
