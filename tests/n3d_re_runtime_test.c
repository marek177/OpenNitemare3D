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
#include "../n3d_re_img.h"
#include "../n3d_re_wall_explosion.h"
#include "../n3d_re_map_archive.h"
#include "../n3d_re_object_defs.h"
#include "../n3d_re_guard_sounds.h"
#include "../n3d_re_guard_perception.h"
#include "../n3d_re_guard_plan.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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

static void write_u32_le(uint8_t* bytes, size_t offset, uint32_t value)
{
    bytes[offset + 0] = (uint8_t)value;
    bytes[offset + 1] = (uint8_t)(value >> 8);
    bytes[offset + 2] = (uint8_t)(value >> 16);
    bytes[offset + 3] = (uint8_t)(value >> 24);
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

    /* WARP_1 is now resolved to the verified 0x0D..0x14 climb-menu family. */
    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES] = 0x90; /* WARP_1 */
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_CLIMB_MENU_REQUEST);
    assert(use_execution.menu_first_wall_type == 0x0D);
    assert(use_execution.menu_last_wall_type == 0x14);
    assert(use_execution.menu_variant_index == 0);

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

    /* WARP_S2 is part of the verified Other Side / mirror family. */
    memset(use_payload, 0, sizeof(use_payload));
    use_payload[adjacent_cell * N3D_MAP_CELL_BYTES] = 0x93;
    assert(N3D_RE_LoadMapPayload(use_payload, sizeof(use_payload)));
    n3d_player.tile_x = 10;
    n3d_player.tile_y = 10;
    use_execution = N3D_RE_ExecuteUse(1);
    assert(use_execution.kind == N3D_USE_EXEC_OTHER_SIDE_REQUEST);
    assert(use_execution.menu_first_wall_type == 0x15);
    assert(use_execution.menu_last_wall_type == 0x18);
    assert(use_execution.menu_variant_index == 1);

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
               N3D_PROJECTILE_COLLISION_HARD_WALL);
        assert(projectile_collision.enter_impact == 1);

        /*
         * Dynamic door: unknown runtime state stays unresolved; known state
         * 1 blocks, while state 0 allows the projectile to continue.
         */
        n3d_map[10 * N3D_MAP_WIDTH + 10].wall = 0x31;
        n3d_wall_mapping_known[0x31] = 1;
        n3d_wall_property_known[0x31] = 1;
        n3d_wall_mapped_type[0x31] = 0x31;
        n3d_wall_property_resolved[0x31] =
            N3D_RE_WallPropertiesForMappedType(0x31);
        assert(N3D_RE_RegisterDoorCell(10, 10) >= 0);

        projectile_collision =
            N3D_RE_ClassifyProjectileCollision(0);
        assert(projectile_collision.kind ==
               N3D_PROJECTILE_COLLISION_UNRESOLVED);

        assert(N3D_RE_SetDoorCellState(10, 10, 1));
        projectile_collision =
            N3D_RE_ClassifyProjectileCollision(0);
        assert(projectile_collision.kind ==
               N3D_PROJECTILE_COLLISION_CLOSED_DOOR);
        assert(projectile_collision.enter_impact == 1);

        assert(N3D_RE_SetDoorCellState(10, 10, 0));
        projectile_collision =
            N3D_RE_ClassifyProjectileCollision(0);
        assert(projectile_collision.kind ==
               N3D_PROJECTILE_COLLISION_NONE);

        /*
         * Generic object blocker 0x02 terminates flight, unless 0x20
         * pass-through is also present.
         */
        n3d_map[10 * N3D_MAP_WIDTH + 10].wall = 0;
        n3d_map[10 * N3D_MAP_WIDTH + 10].object = 0x40;
        n3d_object_mapping_known[0x40] = 1;
        n3d_object_property_known[0x40] = 1;
        n3d_object_mapped_type[0x40] = 0x08;
        n3d_object_property_resolved[0x40] = 0x02;

        projectile_collision =
            N3D_RE_ClassifyProjectileCollision(0);
        assert(projectile_collision.kind ==
               N3D_PROJECTILE_COLLISION_OBJECT_BLOCK);
        assert(projectile_collision.enter_impact == 1);

        n3d_object_property_resolved[0x40] = 0x22;
        projectile_collision =
            N3D_RE_ClassifyProjectileCollision(0);
        assert(projectile_collision.kind ==
               N3D_PROJECTILE_COLLISION_NONE);

        /* Property 0x40 remains an explicit side-effect boundary. */
        n3d_object_property_resolved[0x40] = 0x40;
        projectile_collision =
            N3D_RE_ClassifyProjectileCollision(0);
        assert(projectile_collision.kind ==
               N3D_PROJECTILE_COLLISION_OBJECT_SPECIAL_DEFERRED);
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
               N3D_PROJECTILE_COLLISION_HARD_WALL);
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

        wall_hit.kind = N3D_PROJECTILE_COLLISION_HARD_WALL;
        wall_hit.event_id = 0;
        wall_hit.enter_impact = 1;

        wall_resolution =
            N3D_RE_ResolveProjectileWallCollision(0, &wall_hit);

        assert(wall_resolution.resolved == 1);
        assert(wall_resolution.deferred == 0);
        assert(wall_resolution.event_id == 0);
        assert(wall_resolution.requested_runtime_wall_class == 0);
        assert(wall_resolution.entered_impact == 1);
        assert(n3d_projectiles[0].state == 2);
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

    /* Exact IMG directory + dual 90-byte SEQDEF bank reader. */
    {
        const uint8_t wall_id = 0x20;
        const uint8_t object_id = 0xFB;
        const size_t wall_stream = N3D_IMG_FRAME_DATA_OFFSET;
        const size_t wall_frame_bytes = N3D_IMG_FRAME_HEADER_BYTES + 1;
        const size_t object_stream = wall_stream + 3 * wall_frame_bytes;
        const size_t total_size = object_stream + 2 * wall_frame_bytes;

        uint8_t* img_bytes = (uint8_t*)calloc(total_size, 1);
        assert(img_bytes != NULL);

        write_u32_le(
            img_bytes,
            N3D_IMG_WALL_DIRECTORY_OFFSET + wall_id * 4u,
            (uint32_t)wall_stream);
        write_u32_le(
            img_bytes,
            N3D_IMG_OBJECT_DIRECTORY_OFFSET + object_id * 4u,
            (uint32_t)object_stream);
        assert(N3D_IMG_WALL_DIRECTORY_OFFSET == 0x0000u);
        assert(N3D_IMG_OBJECT_DIRECTORY_OFFSET == 0x0000u);

        const size_t wall_seq =
            N3D_IMG_LOW_SEQUENCE_BANK_OFFSET +
            wall_id * N3D_IMG_SEQUENCE_RECORD_BYTES;
        img_bytes[wall_seq + 0] = 50;
        img_bytes[wall_seq + 1] = 0;
        img_bytes[wall_seq + 2] = 3;
        img_bytes[wall_seq + 3] = 7;
        img_bytes[wall_seq + 4] = 0xA1;

        const size_t object_seq =
            N3D_IMG_HIGH_SEQUENCE_BANK_OFFSET +
            object_id * N3D_IMG_SEQUENCE_RECORD_BYTES;
        img_bytes[object_seq + 0] = 100;
        img_bytes[object_seq + 1] = 0;
        img_bytes[object_seq + 2] = 2;
        img_bytes[object_seq + 3] = 9;
        img_bytes[object_seq + 4] = 0xB2;

        for(unsigned frame = 0; frame < 3; ++frame)
        {
            const size_t off = wall_stream + frame * wall_frame_bytes;
            img_bytes[off + 0] = 1;
            img_bytes[off + 1] = 1;
            img_bytes[off + 2] = (uint8_t)(0x10 + frame);
            img_bytes[off + 9] = (uint8_t)(0x20 + frame);
            img_bytes[off + 10] = (uint8_t)(0x50 + frame);
        }

        for(unsigned frame = 0; frame < 2; ++frame)
        {
            const size_t off = object_stream + frame * wall_frame_bytes;
            img_bytes[off + 0] = 1;
            img_bytes[off + 1] = 1;
            img_bytes[off + 2] = (uint8_t)(0x30 + frame);
            img_bytes[off + 9] = (uint8_t)(0x40 + frame);
            img_bytes[off + 10] = (uint8_t)(0x60 + frame);
        }

        FILE* img_file = fopen("N3D_IMG_SEQDEF_TEST.BIN", "wb");
        assert(img_file != NULL);
        assert(fwrite(img_bytes, 1, total_size, img_file) == total_size);
        fclose(img_file);

        n3d_img_archive archive = {0};
        assert(N3D_RE_LoadImgArchive(
            "N3D_IMG_SEQDEF_TEST.BIN", &archive));
        assert(archive.loaded);
        assert(archive.size == total_size);
        assert(archive.wall_offset[wall_id] == wall_stream);
        assert(archive.object_offset[object_id] == object_stream);
        assert(archive.wall_offset[object_id] == object_stream);
        assert(archive.object_offset[wall_id] == wall_stream);

        const n3d_img_sequence_def* wall_sequence =
            N3D_RE_WallSequence(&archive, wall_id);
        assert(wall_sequence != NULL);
        assert(wall_sequence->interval_ms == 50);
        assert(wall_sequence->frame_count == 3);
        assert(wall_sequence->extended == 7);
        assert(wall_sequence->unknown[0] == 0xA1);

        const n3d_img_sequence_def* object_sequence =
            N3D_RE_ObjectSequence(&archive, object_id);
        assert(object_sequence != NULL);
        assert(object_sequence->interval_ms == 100);
        assert(object_sequence->frame_count == 2);
        assert(object_sequence->extended == 9);
        assert(object_sequence->unknown[0] == 0xB2);

        n3d_img_frame_view frame = {0};
        assert(N3D_RE_WallSequenceFrame(
            &archive, wall_id, 0, &frame));
        assert(frame.file_offset == wall_stream);
        assert(frame.width == 1 && frame.height == 1);
        assert(frame.metadata[0] == 0x10);
        assert(frame.metadata[7] == 0x20);
        assert(frame.pixel_count == 1 && frame.pixels[0] == 0x50);

        assert(N3D_RE_WallSequenceFrame(
            &archive, wall_id, 2, &frame));
        assert(frame.file_offset == wall_stream + 2 * wall_frame_bytes);
        assert(frame.metadata[0] == 0x12);
        assert(frame.pixels[0] == 0x52);
        assert(!N3D_RE_WallSequenceFrame(
            &archive, wall_id, 3, &frame));

        assert(N3D_RE_ObjectSequenceFrame(
            &archive, object_id, 1, &frame));
        assert(frame.file_offset == object_stream + wall_frame_bytes);
        assert(frame.metadata[0] == 0x31);
        assert(frame.pixels[0] == 0x61);
        assert(!N3D_RE_ObjectSequenceFrame(
            &archive, object_id, 2, &frame));

        N3D_RE_FreeImgArchive(&archive);
        assert(!archive.loaded && archive.bytes == NULL);

        /* Reject a directory entry that points into header/SEQDEF space. */
        write_u32_le(
            img_bytes,
            N3D_IMG_WALL_DIRECTORY_OFFSET + wall_id * 4u,
            0x100u);
        img_file = fopen("N3D_IMG_SEQDEF_BAD_TEST.BIN", "wb");
        assert(img_file != NULL);
        assert(fwrite(img_bytes, 1, total_size, img_file) == total_size);
        fclose(img_file);

        assert(!N3D_RE_LoadImgArchive(
            "N3D_IMG_SEQDEF_BAD_TEST.BIN", &archive));

        free(img_bytes);
        remove("N3D_IMG_SEQDEF_TEST.BIN");
        remove("N3D_IMG_SEQDEF_BAD_TEST.BIN");
    }

    /* SEQDEF-driven WALL_EX1/WALL_EX2 explosion lifecycle. */
    {
        const size_t frame_bytes = N3D_IMG_FRAME_HEADER_BYTES + 1;
        const size_t class2d_stream = N3D_IMG_FRAME_DATA_OFFSET;
        const size_t class2f_stream = class2d_stream + 3 * frame_bytes;
        const size_t total_size = class2f_stream + 3 * frame_bytes;
        uint8_t* img_bytes = (uint8_t*)calloc(total_size, 1);
        assert(img_bytes != NULL);

        const uint8_t class2d_id = 0x20;
        const uint8_t wall_ex1_id = 0x21;
        const uint8_t wall_ex2_id = 0x22;

        write_u32_le(
            img_bytes,
            N3D_IMG_WALL_DIRECTORY_OFFSET + class2d_id * 4u,
            (uint32_t)class2d_stream);
        write_u32_le(
            img_bytes,
            N3D_IMG_WALL_DIRECTORY_OFFSET + wall_ex2_id * 4u,
            (uint32_t)class2f_stream);

        const size_t class2d_seq =
            N3D_IMG_LOW_SEQUENCE_BANK_OFFSET +
            class2d_id * N3D_IMG_SEQUENCE_RECORD_BYTES;
        img_bytes[class2d_seq + 0] = 50;
        img_bytes[class2d_seq + 1] = 0;
        img_bytes[class2d_seq + 2] = 3;

        const size_t class2f_seq =
            N3D_IMG_LOW_SEQUENCE_BANK_OFFSET +
            wall_ex2_id * N3D_IMG_SEQUENCE_RECORD_BYTES;
        img_bytes[class2f_seq + 0] = 50;
        img_bytes[class2f_seq + 1] = 0;
        img_bytes[class2f_seq + 2] = 3;

        for(unsigned frame = 0; frame < 3; ++frame)
        {
            size_t off = class2d_stream + frame * frame_bytes;
            img_bytes[off] = 1;
            img_bytes[off + 1] = 1;
            img_bytes[off + 10] = (uint8_t)(0x70 + frame);

            off = class2f_stream + frame * frame_bytes;
            img_bytes[off] = 1;
            img_bytes[off + 1] = 1;
            img_bytes[off + 10] = (uint8_t)(0x80 + frame);
        }

        FILE* f = fopen("N3D_IMG_WALL_EXPLOSION_TEST.BIN", "wb");
        assert(f != NULL);
        assert(fwrite(img_bytes, 1, total_size, f) == total_size);
        fclose(f);
        free(img_bytes);

        assert(N3D_RE_LoadImgArchive(
            "N3D_IMG_WALL_EXPLOSION_TEST.BIN",
            &n3d_img));

        n3d_wall_mapping_known[class2d_id] = 1;
        n3d_wall_property_known[class2d_id] = 1;
        n3d_wall_mapped_type[class2d_id] = 0x2D;
        n3d_wall_property_resolved[class2d_id] =
            N3D_RE_WallPropertiesForMappedType(0x2D);

        n3d_wall_mapping_known[wall_ex1_id] = 1;
        n3d_wall_property_known[wall_ex1_id] = 1;
        n3d_wall_mapped_type[wall_ex1_id] = 0x2E;
        n3d_wall_property_resolved[wall_ex1_id] =
            N3D_RE_WallPropertiesForMappedType(0x2E);

        n3d_wall_mapping_known[wall_ex2_id] = 1;
        n3d_wall_property_known[wall_ex2_id] = 1;
        n3d_wall_mapped_type[wall_ex2_id] = 0x2F;
        n3d_wall_property_resolved[wall_ex2_id] =
            N3D_RE_WallPropertiesForMappedType(0x2F);

        N3D_RE_ResetExplodingWalls();
        n3d_map[5 * N3D_MAP_WIDTH + 4].wall = wall_ex1_id;

        assert(N3D_RE_StartExplodingWall(4, 5, 100));
        const n3d_exploding_wall_record* wall =
            N3D_RE_ExplodingWallAt(4, 5);
        assert(wall != NULL);
        assert(wall->source_wall_id == wall_ex1_id);
        assert(wall->source_wall_class == 0x2E);
        assert(wall->sequence_wall_id == class2d_id);
        assert(wall->frame == 0);
        assert(wall->animation_deadline_ms == 150);

        uint8_t visual_id = 0, visual_frame = 0;
        assert(N3D_RE_ExplodingWallVisual(
            4, 5, &visual_id, &visual_frame));
        assert(visual_id == class2d_id && visual_frame == 0);

        uint8_t runtime_class = 0;
        assert(N3D_RE_ExplodingWallRuntimeClass(
            4, 5, &runtime_class));
        assert(runtime_class == 0x2D);

        assert(N3D_RE_UpdateExplodingWalls(149) == 0);
        assert(N3D_RE_ExplodingWallAt(4, 5)->frame == 0);

        assert(N3D_RE_UpdateExplodingWalls(150) == 0);
        assert(N3D_RE_ExplodingWallAt(4, 5)->frame == 1);
        assert(N3D_RE_ExplodingWallAt(4, 5)->animation_deadline_ms == 200);

        assert(N3D_RE_UpdateExplodingWalls(200) == 0);
        assert(N3D_RE_ExplodingWallAt(4, 5)->frame == 2);

        assert(N3D_RE_UpdateExplodingWalls(250) == 1);
        assert(N3D_RE_ExplodingWallAt(4, 5) == NULL);
        assert(n3d_map[5 * N3D_MAP_WIDTH + 4].wall == 0);

        /* WALL_EX2 keeps its own sequence and starts at frame one. */
        N3D_RE_ResetExplodingWalls();
        n3d_map[5 * N3D_MAP_WIDTH + 4].wall = wall_ex2_id;
        assert(N3D_RE_StartExplodingWall(4, 5, 100));
        wall = N3D_RE_ExplodingWallAt(4, 5);
        assert(wall != NULL);
        assert(wall->sequence_wall_id == wall_ex2_id);
        assert(wall->frame == 1);
        assert(wall->animation_deadline_ms == 150);

        assert(N3D_RE_UpdateExplodingWalls(150) == 0);
        assert(N3D_RE_ExplodingWallAt(4, 5)->frame == 2);
        assert(N3D_RE_UpdateExplodingWalls(200) == 1);
        assert(N3D_RE_ExplodingWallAt(4, 5) == NULL);
        assert(n3d_map[5 * N3D_MAP_WIDTH + 4].wall == 0);

        /*
         * Projectile resolver starts the wall lifecycle and enters impact.
         * While active, projectile collision sees runtime class 0x2D, so a
         * second hit does not retrigger the explodable-property branch.
         */
        N3D_RE_ResetExplodingWalls();
        n3d_map[5 * N3D_MAP_WIDTH + 4].wall = wall_ex1_id;
        assert(N3D_RE_InitializeProjectile(
            0, N3D_WEAPON_SINGLE_LASER,
            4 * 64 + 32, 5 * 64 + 32, 20));

        n3d_projectile_collision_result collision =
            N3D_RE_ClassifyProjectileCollision(0);
        assert(collision.kind ==
               N3D_PROJECTILE_COLLISION_EXPLODABLE_WALL);

        n3d_projectile_wall_resolution wall_resolution =
            N3D_RE_ResolveProjectileWallCollisionAtTime(
                0, &collision, 100);
        assert(wall_resolution.resolved == 1);
        assert(wall_resolution.explosion_started == 1);
        assert(wall_resolution.event_id == 0x29);
        assert(wall_resolution.entered_impact == 1);
        assert(n3d_projectiles[0].state == 2);
        assert(N3D_RE_ExplodingWallAt(4, 5) != NULL);

        assert(N3D_RE_InitializeProjectile(
            1, N3D_WEAPON_SINGLE_LASER,
            4 * 64 + 32, 5 * 64 + 32, 20));
        collision = N3D_RE_ClassifyProjectileCollision(1);
        assert(collision.kind ==
               N3D_PROJECTILE_COLLISION_HARD_WALL);
        assert(collision.mapped_wall_type == 0x2D);
        assert(collision.enter_impact == 1);

        N3D_RE_FreeImgArchive(&n3d_img);
        N3D_RE_ResetExplodingWalls();
        remove("N3D_IMG_WALL_EXPLOSION_TEST.BIN");
    }

    /* Special-wall USE families: exact handler classification only. */
    {
        uint8_t special_use_payload[N3D_MAP_LEVEL_BYTES] = {0};
        const int special_adjacent = 10 * N3D_MAP_WIDTH + 11;

        /* Wall type 8 -> scripted Episode-1 interaction family. */
        n3d_wall_mapping_known[0xA0] = 1;
        n3d_wall_property_known[0xA0] = 1;
        n3d_wall_mapped_type[0xA0] = 0x08;
        n3d_wall_property_resolved[0xA0] =
            N3D_RE_WallPropertiesForMappedType(0x08);

        special_use_payload[special_adjacent * N3D_MAP_CELL_BYTES] = 0xA0;
        assert(N3D_RE_LoadMapPayload(
            special_use_payload, sizeof(special_use_payload)));
        n3d_player.tile_x = 10;
        n3d_player.tile_y = 10;

        n3d_use_execution special_use =
            N3D_RE_ExecuteUse(1);
        assert(special_use.kind ==
               N3D_USE_EXEC_SCRIPTED_WALL_REQUEST);

        /* 0x0D..0x14 -> climb up/down menu family. */
        memset(special_use_payload, 0, sizeof(special_use_payload));
        n3d_wall_mapping_known[0xA1] = 1;
        n3d_wall_property_known[0xA1] = 1;
        n3d_wall_mapped_type[0xA1] = 0x10;
        n3d_wall_property_resolved[0xA1] =
            N3D_RE_WallPropertiesForMappedType(0x10);
        special_use_payload[special_adjacent * N3D_MAP_CELL_BYTES] = 0xA1;

        assert(N3D_RE_LoadMapPayload(
            special_use_payload, sizeof(special_use_payload)));
        n3d_player.tile_x = 10;
        n3d_player.tile_y = 10;

        special_use = N3D_RE_ExecuteUse(1);
        assert(special_use.kind ==
               N3D_USE_EXEC_CLIMB_MENU_REQUEST);
        assert(special_use.menu_first_wall_type == 0x0D);
        assert(special_use.menu_last_wall_type == 0x14);
        assert(special_use.menu_variant_index == 3);

        /* 0x16..0x18 -> Other Side/mirror family request. */
        memset(special_use_payload, 0, sizeof(special_use_payload));
        n3d_wall_mapping_known[0xA2] = 1;
        n3d_wall_property_known[0xA2] = 1;
        n3d_wall_mapped_type[0xA2] = 0x16;
        n3d_wall_property_resolved[0xA2] =
            N3D_RE_WallPropertiesForMappedType(0x16);
        special_use_payload[special_adjacent * N3D_MAP_CELL_BYTES] = 0xA2;

        assert(N3D_RE_LoadMapPayload(
            special_use_payload, sizeof(special_use_payload)));
        n3d_player.tile_x = 10;
        n3d_player.tile_y = 10;

        special_use = N3D_RE_ExecuteUse(1);
        assert(special_use.kind ==
               N3D_USE_EXEC_OTHER_SIDE_REQUEST);
        assert(special_use.menu_first_wall_type == 0x15);
        assert(special_use.menu_last_wall_type == 0x18);
        assert(special_use.menu_variant_index == 1);

        /* 0x1D..0x24 -> Floor 1..10 selector family. */
        memset(special_use_payload, 0, sizeof(special_use_payload));
        n3d_wall_mapping_known[0xA3] = 1;
        n3d_wall_property_known[0xA3] = 1;
        n3d_wall_mapped_type[0xA3] = 0x20;
        n3d_wall_property_resolved[0xA3] =
            N3D_RE_WallPropertiesForMappedType(0x20);
        special_use_payload[special_adjacent * N3D_MAP_CELL_BYTES] = 0xA3;

        assert(N3D_RE_LoadMapPayload(
            special_use_payload, sizeof(special_use_payload)));
        n3d_player.tile_x = 10;
        n3d_player.tile_y = 10;

        special_use = N3D_RE_ExecuteUse(1);
        assert(special_use.kind ==
               N3D_USE_EXEC_FLOOR_MENU_REQUEST);
        assert(special_use.menu_first_wall_type == 0x1D);
        assert(special_use.menu_last_wall_type == 0x24);
        assert(special_use.menu_variant_index == 3);

        /* 0x25..0x2C -> Go down / Cancel family. */
        memset(special_use_payload, 0, sizeof(special_use_payload));
        n3d_wall_mapping_known[0xA4] = 1;
        n3d_wall_property_known[0xA4] = 1;
        n3d_wall_mapped_type[0xA4] = 0x29;
        n3d_wall_property_resolved[0xA4] =
            N3D_RE_WallPropertiesForMappedType(0x29);
        special_use_payload[special_adjacent * N3D_MAP_CELL_BYTES] = 0xA4;

        assert(N3D_RE_LoadMapPayload(
            special_use_payload, sizeof(special_use_payload)));
        n3d_player.tile_x = 10;
        n3d_player.tile_y = 10;

        special_use = N3D_RE_ExecuteUse(1);
        assert(special_use.kind ==
               N3D_USE_EXEC_GO_DOWN_MENU_REQUEST);
        assert(special_use.menu_first_wall_type == 0x25);
        assert(special_use.menu_last_wall_type == 0x2C);
        assert(special_use.menu_variant_index == 4);

        /*
         * Wall type 3 remote terminal: associated OBJECT+01 selects ID-card
         * bit. Build a synthetic runtime object bound to the same cell.
         */
        N3D_RE_ResetRuntime();
        n3d_wall_mapping_known[0xA5] = 1;
        n3d_wall_property_known[0xA5] = 1;
        n3d_wall_mapped_type[0xA5] = 0x03;
        n3d_wall_property_resolved[0xA5] =
            N3D_RE_WallPropertiesForMappedType(0x03);

        n3d_object_mapping_known[0xB0] = 1;
        n3d_object_property_known[0xB0] = 1;
        n3d_object_mapped_type[0xB0] = 0x06;
        n3d_object_property_resolved[0xB0] =
            N3D_RE_ObjectPropertiesForMappedType(0x06);

        n3d_map[special_adjacent].wall = 0xA5;
        n3d_map[special_adjacent].object = 0xB0;

        int terminal_object_slot = -1;
        assert(N3D_RE_InstantiateMapObject(
            0xB0, 11, 10, &terminal_object_slot));
        assert(terminal_object_slot >= 0);

        n3d_objects[terminal_object_slot].variant = 1;
        n3d_player.tile_x = 10;
        n3d_player.tile_y = 10;
        n3d_player.id_cards = 0;

        special_use = N3D_RE_ExecuteUse(1);
        assert(special_use.kind ==
               N3D_USE_EXEC_REMOTE_TERMINAL_BLOCKED);
        assert(special_use.required_inventory_bit == 1);
        assert(special_use.runtime_slot == terminal_object_slot);

        n3d_player.id_cards = 0x02;
        special_use = N3D_RE_ExecuteUse(1);
        assert(special_use.kind ==
               N3D_USE_EXEC_REMOTE_TERMINAL_PASSED);
        assert(special_use.required_inventory_bit == 1);
        assert(special_use.runtime_slot == terminal_object_slot);
    }

    /* Exact 514-byte MAP archive header class tables. */
    {
        uint8_t map_bytes[
            N3D_MAP_HEADER_BYTES + 2 * N3D_MAP_LEVEL_BYTES];
        memset(map_bytes, 0, sizeof(map_bytes));

        map_bytes[0] = 2;
        map_bytes[1] = 0;

        /* Wall classes: class-44 AREA run + exploding wall families. */
        map_bytes[0x002 + 10] = 0x44;
        map_bytes[0x002 + 11] = 0x44;
        map_bytes[0x002 + 15] = 0x44;
        map_bytes[0x002 + 0x20] = 0x2D;
        map_bytes[0x002 + 0x21] = 0x2E;
        map_bytes[0x002 + 0x22] = 0x2F;
        map_bytes[0x002 + 0x31] = 0x31;

        /* Object classes include the unnamed projectile template class 0x05. */
        map_bytes[0x102 + 0xFB] = 0x05;
        map_bytes[0x102 + 0xFC] = 0x05;
        map_bytes[0x102 + 0xFD] = 0x05;
        map_bytes[0x102 + 0xFE] = 0x05;
        map_bytes[0x102 + 0x80] = 0x08;
        map_bytes[0x102 + 0x81] = 0x08;

        FILE* map_file =
            fopen("N3D_MAP_HEADER_TEST.BIN", "wb");
        assert(map_file != NULL);
        assert(fwrite(
            map_bytes,
            1,
            sizeof(map_bytes),
            map_file) == sizeof(map_bytes));
        fclose(map_file);

        n3d_map_archive_header header = {0};
        assert(N3D_RE_LoadMapArchiveHeader(
            "N3D_MAP_HEADER_TEST.BIN",
            &header));
        assert(header.loaded);
        assert(header.level_count == 2);
        assert(header.wall_class[10] == 0x44);
        assert(header.wall_class[0x21] == 0x2E);
        assert(header.object_class[0xFB] == 0x05);
        assert(header.object_class[0x80] == 0x08);

        uint8_t raw_id = 0;
        assert(N3D_RE_FindFirstWallIdForClass(
            &header, 0x44, &raw_id));
        assert(raw_id == 10);
        assert(N3D_RE_FindFirstWallIdForClass(
            &header, 0x2D, &raw_id));
        assert(raw_id == 0x20);

        assert(N3D_RE_FindFirstObjectIdForClass(
            &header, 0x05, &raw_id));
        assert(raw_id == 0xFB);

        uint8_t variant = 0xFF;
        assert(N3D_RE_MapWallClassVariant(
            &header, 15, 0x44, &variant));
        assert(variant == 5);
        assert(!N3D_RE_MapWallClassVariant(
            &header, 15, 0x43, &variant));

        uint8_t object_class = 0;
        assert(N3D_RE_MapObjectClassVariant(
            &header, 0xFE, &object_class, &variant));
        assert(object_class == 0x05);
        assert(variant == 3);

        /* Exact class tables supersede text-name/bootstrap guesses. */
        memset(n3d_wall_mapping_known, 0, sizeof(n3d_wall_mapping_known));
        memset(n3d_object_mapping_known, 0, sizeof(n3d_object_mapping_known));
        memset(n3d_wall_property_known, 0, sizeof(n3d_wall_property_known));
        memset(n3d_object_property_known, 0, sizeof(n3d_object_property_known));

        N3D_RE_ApplyMapArchiveClassTables(&header);

        for(int id = 0; id < 256; ++id)
        {
            assert(n3d_wall_mapping_known[id] == 1);
            assert(n3d_object_mapping_known[id] == 1);
            assert(n3d_wall_property_known[id] == 1);
            assert(n3d_object_property_known[id] == 1);
            assert(n3d_wall_mapped_type[id] == header.wall_class[id]);
            assert(n3d_object_mapped_type[id] == header.object_class[id]);
            assert(n3d_wall_property_resolved[id] ==
                   N3D_RE_WallPropertiesForMappedType(
                       header.wall_class[id]));
            assert(n3d_object_property_resolved[id] ==
                   N3D_RE_ObjectPropertiesForMappedType(
                       header.object_class[id]));
        }

        /* AREA lookup now works for anonymous class-table entries too. */
        assert(N3D_RE_AreaIdFromWallId(15, &variant));
        assert(variant == 5);

        remove("N3D_MAP_HEADER_TEST.BIN");
    }

    /* Byte-exact 0x5A object-resource definition catalog. */
    {
        const uint8_t object_a = 0x80;
        const uint8_t object_b = 0x81;
        const uint8_t object_c = 0x82;

        const size_t stream_a =
            N3D_IMG_FRAME_DATA_OFFSET;
        const size_t stream_c =
            stream_a +
            2 * (N3D_IMG_FRAME_HEADER_BYTES + 1);
        const size_t total_size =
            stream_c +
            (N3D_IMG_FRAME_HEADER_BYTES + 1);

        uint8_t* img_bytes =
            (uint8_t*)calloc(total_size, 1);
        assert(img_bytes != NULL);

        /* A/B share one sourceKey and therefore one runtime definition id. */
        write_u32_le(
            img_bytes,
            N3D_IMG_RESOURCE_DIRECTORY_OFFSET + object_a * 4u,
            (uint32_t)stream_a);
        write_u32_le(
            img_bytes,
            N3D_IMG_RESOURCE_DIRECTORY_OFFSET + object_b * 4u,
            (uint32_t)stream_a);
        write_u32_le(
            img_bytes,
            N3D_IMG_RESOURCE_DIRECTORY_OFFSET + object_c * 4u,
            (uint32_t)stream_c);

        const size_t header_a =
            N3D_IMG_HIGH_SEQUENCE_BANK_OFFSET +
            object_a * N3D_OBJECT_RESOURCE_HEADER_BYTES;
        const size_t header_b =
            N3D_IMG_HIGH_SEQUENCE_BANK_OFFSET +
            object_b * N3D_OBJECT_RESOURCE_HEADER_BYTES;
        const size_t header_c =
            N3D_IMG_HIGH_SEQUENCE_BANK_OFFSET +
            object_c * N3D_OBJECT_RESOURCE_HEADER_BYTES;

        img_bytes[header_a + 0x02] = 2;
        img_bytes[header_b + 0x02] = 2;
        img_bytes[header_c + 0x02] = 1;

        /* Directional A/B/C and state 02/03/04 words. */
        img_bytes[header_a + 0x04] = 0x10;
        img_bytes[header_a + 0x05] = 0x01;
        img_bytes[header_a + 0x14] = 0x20;
        img_bytes[header_a + 0x15] = 0x02;
        img_bytes[header_a + 0x24] = 0x30;
        img_bytes[header_a + 0x25] = 0x03;

        img_bytes[header_a + 0x34] = 0x40;
        img_bytes[header_a + 0x35] = 0x04;
        img_bytes[header_a + 0x36] = 0x50;
        img_bytes[header_a + 0x37] = 0x05;
        img_bytes[header_a + 0x38] = 0x60;
        img_bytes[header_a + 0x39] = 0x06;

        img_bytes[header_a + 0x3A] = 0x70;
        img_bytes[header_a + 0x3B] = 0x07;
        img_bytes[header_a + 0x4A] = 0x80;
        img_bytes[header_a + 0x4B] = 0x08;

        /*
         * B deliberately has different header bytes but same sourceKey:
         * original catalog dedup keeps the first registered definition.
         */
        memcpy(
            img_bytes + header_b,
            img_bytes + header_a,
            N3D_OBJECT_RESOURCE_HEADER_BYTES);
        img_bytes[header_b + 0x34] = 0x99;

        img_bytes[header_c + 0x34] = 0xA0;
        img_bytes[header_c + 0x35] = 0x01;

        for(unsigned frame = 0; frame < 2; ++frame)
        {
            const size_t off =
                stream_a +
                frame *
                (N3D_IMG_FRAME_HEADER_BYTES + 1);
            img_bytes[off + 0] = 1;
            img_bytes[off + 1] = 1;
            img_bytes[off + 10] =
                (uint8_t)(0x40 + frame);
        }

        img_bytes[stream_c + 0] = 1;
        img_bytes[stream_c + 1] = 1;
        img_bytes[stream_c + 10] = 0x55;

        FILE* f =
            fopen("N3D_OBJECT_DEFS_TEST.BIN", "wb");
        assert(f != NULL);
        assert(fwrite(
            img_bytes,
            1,
            total_size,
            f) == total_size);
        fclose(f);
        free(img_bytes);

        N3D_RE_FreeImgArchive(&n3d_img);
        assert(N3D_RE_LoadImgArchive(
            "N3D_OBJECT_DEFS_TEST.BIN",
            &n3d_img));

        N3D_RE_ResetObjectDefinitions();

        uint8_t def_a = 0xFF;
        uint8_t def_b = 0xFF;
        uint8_t def_c = 0xFF;

        assert(N3D_RE_RegisterMapObjectDefinition(
            object_a, &def_a));
        assert(def_a == 0);
        assert(n3d_object_definition_count == 1);

        assert(N3D_RE_RegisterMapObjectDefinition(
            object_b, &def_b));
        assert(def_b == def_a);
        assert(n3d_object_definition_count == 1);

        assert(N3D_RE_RegisterMapObjectDefinition(
            object_c, &def_c));
        assert(def_c == 1);
        assert(n3d_object_definition_count == 2);

        const n3d_object_definition_slot* def =
            N3D_RE_ObjectDefinition(def_a);
        assert(def != NULL);
        assert(def->source_key == stream_a);
        assert(def->source_object_id == object_a);
        assert(N3D_RE_ObjectDefinitionFrameCount(
            &def->header) == 2);

        assert(N3D_RE_ObjectDefinitionDirectional(
            &def->header, 0, 0) == 0x0110);
        assert(N3D_RE_ObjectDefinitionDirectional(
            &def->header, 1, 0) == 0x0220);
        assert(N3D_RE_ObjectDefinitionDirectional(
            &def->header, 2, 0) == 0x0330);

        uint16_t sequence = 0;
        assert(N3D_RE_ObjectDefinitionGuardStateSequence(
            &def->header,
            N3D_GUARD_STATE_02,
            &sequence));
        assert(sequence == 0x0440);
        assert(N3D_RE_PackedSequenceHasFrames(sequence));

        assert(N3D_RE_ObjectDefinitionGuardStateSequence(
            &def->header,
            N3D_GUARD_STATE_03,
            &sequence));
        assert(sequence == 0x0550);

        assert(N3D_RE_ObjectDefinitionGuardStateSequence(
            &def->header,
            N3D_GUARD_STATE_04,
            &sequence));
        assert(sequence == 0x0660);

        assert(N3D_RE_ObjectDefinitionReactionSequence(
            &def->header, 0) == 0x0770);
        assert(N3D_RE_ObjectDefinitionDeathSequence(
            &def->header, 0) == 0x0880);

        /* Register through normal OBJECT instantiation. */
        n3d_object_mapping_known[object_a] = 1;
        n3d_object_property_known[object_a] = 1;
        n3d_object_mapped_type[object_a] = 0x08;
        n3d_object_property_resolved[object_a] =
            N3D_RE_ObjectPropertiesForMappedType(0x08);

        N3D_RE_ResetRuntime();

        /* ResetRuntime clears catalog, not the loaded IMG archive. */
        assert(n3d_object_definition_count == 0);
        assert(N3D_RE_InstantiateMapObject(
            object_a, 3, 4, NULL));
        assert(n3d_object_count == 1);
        assert(n3d_object_definition_count == 1);
        assert(n3d_objects[0].sequence_id == 0);

        def = N3D_RE_ObjectDefinition(
            n3d_objects[0].sequence_id);
        assert(def != NULL);
        assert(N3D_RE_ObjectDefinitionGuardStateSequence(
            &def->header,
            N3D_GUARD_STATE_02,
            &sequence));
        assert(sequence == 0x0440);

        N3D_RE_FreeImgArchive(&n3d_img);
        N3D_RE_ResetObjectDefinitions();
        remove("N3D_OBJECT_DEFS_TEST.BIN");
    }

    /* Exact 125-ms GUARD logic clock; no catch-up replay. */
    {
        N3D_RE_ResetGuardLogicClock();
        assert(n3d_guard_clock.accumulator_ms == 0);
        assert(n3d_guard_clock.tick_due == 0);

        assert(!N3D_RE_BeginGuardLogicFrame(0));
        assert(!N3D_RE_BeginGuardLogicFrame(40));
        assert(n3d_guard_clock.accumulator_ms == 40);
        assert(!N3D_RE_BeginGuardLogicFrame(40));
        assert(n3d_guard_clock.accumulator_ms == 80);
        assert(!N3D_RE_BeginGuardLogicFrame(40));
        assert(n3d_guard_clock.accumulator_ms == 120);

        assert(N3D_RE_BeginGuardLogicFrame(5));
        assert(n3d_guard_clock.tick_due == 1);
        assert(n3d_guard_clock.accumulator_ms == 0);

        /* 250 ms is still one dispatch, not two catch-up ticks. */
        assert(N3D_RE_BeginGuardLogicFrame(250));
        assert(n3d_guard_clock.tick_due == 1);
        assert(n3d_guard_clock.accumulator_ms == 0);

        assert(N3D_RE_BeginGuardLogicFrame(130));
        assert(n3d_guard_clock.accumulator_ms == 5);
        assert(!N3D_RE_BeginGuardLogicFrame(119));
        assert(n3d_guard_clock.accumulator_ms == 124);
        assert(N3D_RE_BeginGuardLogicFrame(1));
        assert(n3d_guard_clock.accumulator_ms == 0);

        N3D_RE_ResetGuardLogicClock();
        assert(!n3d_guard_clock.tick_due);
        assert(n3d_guard_clock.accumulator_ms == 0);
    }

    /* Packed GUARD state-00/02/03/04 sequence transitions. */
    {
        n3d_guard_record guard = {0};
        n3d_object_record object = {0};

        guard.state = N3D_GUARD_STATE_02;
        assert(N3D_RE_BeginState02AlertSequence(
            &guard, &object, 0x0342));
        assert(guard.definition_value == 0x0342);
        assert((uint8_t)object.animation_frame == 0x42);
        assert(guard.timer == 2);
        assert(guard.next_state == N3D_GUARD_STATE_03);
        assert(guard.state == N3D_GUARD_STATE_00);

        assert(N3D_RE_TickAnimationTimer(
            &guard, &object) == 1);
        assert((uint8_t)object.animation_frame == 0x43);
        assert(guard.timer == 1);
        assert(guard.state == N3D_GUARD_STATE_00);

        assert(N3D_RE_TickAnimationTimer(
            &guard, &object) == 2);
        assert((uint8_t)object.animation_frame == 0x44);
        assert(guard.timer == 0);
        assert(guard.state == N3D_GUARD_STATE_03);

        assert(N3D_RE_BeginState03AttackSequence(
            &guard, &object, 0x0250));
        assert(guard.definition_value == 0x0250);
        assert((uint8_t)object.animation_frame == 0x50);
        assert(guard.timer == 1);
        assert(guard.next_state == N3D_GUARD_STATE_04);
        assert(guard.state == N3D_GUARD_STATE_00);

        assert(N3D_RE_TickAnimationTimer(
            &guard, &object) == 2);
        assert((uint8_t)object.animation_frame == 0x51);
        assert(guard.state == N3D_GUARD_STATE_04);

        assert(N3D_RE_BeginState04RecoverySequence(
            &guard, &object, 0x0330));
        assert(guard.definition_value == 0x0330);
        assert((uint8_t)object.animation_frame == 0x30);
        assert(guard.timer == 2);
        assert(guard.next_state == N3D_GUARD_STATE_05);
        assert(guard.state == N3D_GUARD_STATE_00);

        /* Generic state-00 looping uses packed first-frame/frame-count. */
        guard.definition_value = 0x0320;
        guard.timer = 4;
        guard.next_state = N3D_GUARD_STATE_07;
        guard.state = N3D_GUARD_STATE_00;
        object.animation_frame = 0x22;

        assert(N3D_RE_TickAnimationTimer(
            &guard, &object) == 1);
        assert((uint8_t)object.animation_frame == 0x20);
        assert(guard.timer == 3);

        assert(N3D_RE_TickAnimationTimer(
            &guard, &object) == 1);
        assert((uint8_t)object.animation_frame == 0x21);

        assert(N3D_RE_TickAnimationTimer(
            &guard, &object) == 1);
        assert((uint8_t)object.animation_frame == 0x22);

        assert(N3D_RE_TickAnimationTimer(
            &guard, &object) == 2);
        assert((uint8_t)object.animation_frame == 0x20);
        assert(guard.state == N3D_GUARD_STATE_07);
    }

    /* Exact class-specific alert/attack/death SND selectors and RNG classes. */
    {
        assert(!N3D_RE_GuardAlertUsesRandom(0x08));
        assert(N3D_RE_GuardAlertUsesRandom(0x09));
        assert(N3D_RE_GuardAlertUsesRandom(0x0A));
        assert(N3D_RE_GuardAlertUsesRandom(0x0F));
        assert(N3D_RE_GuardAlertUsesRandom(0x10));

        assert(N3D_RE_GuardAlertSoundId(0x08, 0) == 0x22);
        assert(N3D_RE_GuardAlertSoundId(0x09, 0) == 0x38);
        assert(N3D_RE_GuardAlertSoundId(0x09, 1) == 0x39);
        assert(N3D_RE_GuardAlertSoundId(0x09, 2) == 0x3A);
        assert(N3D_RE_GuardAlertSoundId(0x0F, 0) == 0x36);
        assert(N3D_RE_GuardAlertSoundId(0x0F, 1) == 0x37);

        assert(N3D_RE_GuardAttackDamageUsesRandom(0x16));
        assert(N3D_RE_GuardAttackDamageUsesRandom(0x1B));
        assert(!N3D_RE_GuardAttackDamageUsesRandom(0x12));
        assert(N3D_RE_GuardAttackSoundId(0x16, 2) == 0x19);
        assert(N3D_RE_GuardAttackSoundId(0x1B, 3) == 0x4E);

        assert(N3D_RE_GuardDeathUsesRandom(0x0F));
        assert(!N3D_RE_GuardDeathUsesRandom(0x0E));
        assert(N3D_RE_GuardDeathSoundId(0x0F, 2) == 0x0D);
        assert(N3D_RE_GuardDeathSoundId(0x08, 0) == 0x23);

        /* RNG consumption parity for state-02 alert classes. */
        N3D_RE_ResetOriginalRng();
        const uint16_t alert_random =
            N3D_RE_GuardAlertUsesRandom(0x09)
                ? N3D_RE_RngNextGlobal()
                : 0;
        assert(alert_random == 41);
        assert(N3D_RE_GuardAlertSoundId(
            0x09, alert_random) == 0x3A);
    }

    /* Exact GUARD 7494 + D50A + 7594 perception/attack helpers. */
    {
        n3d_guard_record guard = {0};
        n3d_object_record object = {0};

        object.world_x = 10 * 64 + 32;
        object.world_y = 10 * 64 + 32;
        guard.octant = 2; /* east */

        assert(N3D_RE_GuardPerceptionPrefilter(
            &guard, &object, 11 * 64 + 32, 10 * 64 + 32, 0));
        assert(!N3D_RE_GuardPerceptionPrefilter(
            &guard, &object, 9 * 64 + 32, 10 * 64 + 32, 0));
        assert(N3D_RE_GuardPerceptionPrefilter(
            &guard, &object, 9 * 64 + 32, 10 * 64 + 32, 1));
        assert(!N3D_RE_GuardPerceptionPrefilter(
            &guard, &object, 19 * 64 + 32, 10 * 64 + 32, 1));

        assert(N3D_RE_GuardLosCellBlocks(0x06, 0, 0, 1));
        assert(N3D_RE_GuardLosCellBlocks(0x0A, 0, 0, 0));
        assert(!N3D_RE_GuardLosCellBlocks(0x0A, 0, 0, 1));
        assert(N3D_RE_GuardLosCellBlocks(0, 0x02, 1, 1));
        assert(!N3D_RE_GuardLosCellBlocks(0, 0x22, 1, 1));
        assert(!N3D_RE_GuardLosCellBlocks(0, 0x02, 0, 1));

        N3D_RE_ResetRuntime();
        for(int id = 0; id < 256; ++id)
        {
            n3d_wall_mapping_known[id] = 1;
            n3d_wall_property_known[id] = 1;
            n3d_object_mapping_known[id] = 1;
            n3d_object_property_known[id] = 1;
            n3d_wall_mapped_type[id] = 0;
            n3d_object_mapped_type[id] = 0;
            n3d_wall_property_resolved[id] = 0;
            n3d_object_property_resolved[id] = 0;
        }

        n3d_wall_mapped_type[1] = 0x01;
        n3d_wall_property_resolved[1] =
            N3D_RE_WallPropertiesForMappedType(0x01);

        /* Destination wall is intentionally not tested by D50A callback. */
        n3d_map[10 * 64 + 12].wall = 1;
        assert(N3D_RE_TraceGuardGridLine(
            10, 10, 2, 0, 8, 1,
            N3D_RE_GuardMapIntermediateBlocked, NULL));

        n3d_map[10 * 64 + 11].wall = 1;
        assert(!N3D_RE_TraceGuardGridLine(
            10, 10, 2, 0, 8, 1,
            N3D_RE_GuardMapIntermediateBlocked, NULL));

        /* Door state unknown blocks; 0 opens the intermediate LOS cell. */
        n3d_map[10 * 64 + 11].wall = 2;
        n3d_wall_mapped_type[2] = 0x31;
        n3d_wall_property_resolved[2] =
            N3D_RE_WallPropertiesForMappedType(0x31);
        assert(N3D_RE_RegisterDoorCell(11, 10) >= 0);
        assert(!N3D_RE_TraceGuardGridLine(
            10, 10, 2, 0, 8, 1,
            N3D_RE_GuardMapIntermediateBlocked, NULL));
        assert(N3D_RE_SetDoorCellState(11, 10, 0));
        assert(N3D_RE_TraceGuardGridLine(
            10, 10, 2, 0, 8, 1,
            N3D_RE_GuardMapIntermediateBlocked, NULL));

        /* Secondary bit 0x02 blocks only with checks enabled; 0x20 bypasses. */
        n3d_map[10 * 64 + 11].wall = 0;
        n3d_map[10 * 64 + 11].object = 3;
        n3d_object_property_resolved[3] = 0x02;
        assert(!N3D_RE_TraceGuardGridLine(
            10, 10, 2, 0, 8, 1,
            N3D_RE_GuardMapIntermediateBlocked, NULL));
        assert(N3D_RE_TraceGuardGridLine(
            10, 10, 2, 0, 8, 0,
            N3D_RE_GuardMapIntermediateBlocked, NULL));
        n3d_object_property_resolved[3] = 0x22;
        assert(N3D_RE_TraceGuardGridLine(
            10, 10, 2, 0, 8, 1,
            N3D_RE_GuardMapIntermediateBlocked, NULL));

        /* Full 7494+D50A wrapper. */
        guard.octant = 2;
        object.world_x = 10 * 64 + 32;
        object.world_y = 10 * 64 + 32;
        n3d_map[10 * 64 + 11].object = 0;
        assert(N3D_RE_EvaluateGuardPerceptionMap(
            &guard, &object, 12 * 64 + 32, 10 * 64 + 32, 1, 0));
        n3d_map[10 * 64 + 11].wall = 1;
        assert(!N3D_RE_EvaluateGuardPerceptionMap(
            &guard, &object, 12 * 64 + 32, 10 * 64 + 32, 1, 0));

        /* FUN_7594 caches both results and selects by +0x16. */
        int attack_eligible = -1;
        guard.transition_flag = 0;
        assert(N3D_RE_TryEvaluateGuardAttackGate(
            &guard, &object, 11 * 64 + 32, 10 * 64 + 32, 0,
            &attack_eligible));
        assert(attack_eligible == 1);
        assert(guard.unknown_17 == 0 && guard.unknown_18 == 1);

        guard.transition_flag = 1;
        assert(N3D_RE_TryEvaluateGuardAttackGate(
            &guard, &object, 20 * 64, 20 * 64, 1, &attack_eligible));
        assert(attack_eligible == 1);
        assert(guard.unknown_17 == 1 && guard.unknown_18 == 0);

        guard.transition_flag = 2;
        assert(N3D_RE_TryEvaluateGuardAttackGate(
            &guard, &object, 20 * 64, 20 * 64, 0, &attack_eligible));
        assert(attack_eligible == 0);

        guard.transition_flag = 3;
        assert(!N3D_RE_TryEvaluateGuardAttackGate(
            &guard, &object, object.world_x, object.world_y, 1,
            &attack_eligible));
        assert(attack_eligible == 0);

        /* Spawn profile is actually written to +0x16. */
        N3D_RE_ResetRuntime();
        n3d_object_mapping_known[0x80] = 1;
        n3d_object_property_known[0x80] = 1;
        n3d_object_mapped_type[0x80] = 0x08;
        n3d_object_property_resolved[0x80] =
            N3D_RE_ObjectPropertiesForMappedType(0x08);
        assert(N3D_RE_RegisterGuardFromMap(0x80, 1, 1));
        assert(n3d_guards[0].transition_flag == 0);

        N3D_RE_ResetRuntime();
        n3d_object_mapping_known[0x90] = 1;
        n3d_object_property_known[0x90] = 1;
        n3d_object_mapped_type[0x90] = 0x0B;
        n3d_object_property_resolved[0x90] =
            N3D_RE_ObjectPropertiesForMappedType(0x0B);
        assert(N3D_RE_InstantiateMapObject(0x90, 1, 1, NULL));
        assert(n3d_guard_count == 1);
        assert(n3d_guards[0].transition_flag == 1);
    }

    /* Exact FUN_76FC strategy movement planner + RNG consumption. */
    {
        n3d_guard_record guard = {0};
        n3d_object_record object = {0};

        object.world_x = 128;
        object.world_y = 128;

        assert(N3D_RE_GuardSignedStepFromDelta(-1) == -8);
        assert(N3D_RE_GuardSignedStepFromDelta(0) == 0);
        assert(N3D_RE_GuardSignedStepFromDelta(1) == 8);

        guard.octant = 4;
        N3D_RE_UpdateGuardOctantFromMovement(&guard, 8, -8);
        assert(guard.octant == 1);
        N3D_RE_UpdateGuardOctantFromMovement(&guard, 8, 0);
        assert(guard.octant == 2);
        N3D_RE_UpdateGuardOctantFromMovement(&guard, 8, 8);
        assert(guard.octant == 3);
        N3D_RE_UpdateGuardOctantFromMovement(&guard, 0, 8);
        assert(guard.octant == 4);
        N3D_RE_UpdateGuardOctantFromMovement(&guard, -8, 8);
        assert(guard.octant == 5);
        N3D_RE_UpdateGuardOctantFromMovement(&guard, -8, 0);
        assert(guard.octant == 6);
        N3D_RE_UpdateGuardOctantFromMovement(&guard, -8, -8);
        assert(guard.octant == 7);
        N3D_RE_UpdateGuardOctantFromMovement(&guard, 0, -8);
        assert(guard.octant == 0);

        /*
         * Strategy 0: unknown17=0 means first RNG is &3, timer fixed 0x18,
         * so only one RNG value is consumed.
         */
        memset(&guard, 0, sizeof(guard));
        guard.strategy = 0;
        guard.unknown_17 = 0;
        guard.unknown_18 = 0;
        uint32_t rng = 1;
        uint32_t expected_rng = rng;
        const uint16_t first_rand =
            N3D_RE_RngNext(&expected_rng);
        assert(first_rand == 41);

        assert(N3D_RE_PlanStrategy0MovementWithRng(
            &guard, &object,
            64, 64,
            1, &rng));
        assert(rng == expected_rng);
        assert(guard.timer == 0x18);
        assert(guard.state == N3D_GUARD_STATE_06);
        assert(guard.octant <= 7);

        /*
         * With cached perception and no proximity, pursuit consumes a second
         * RNG for timer 8..15 and applies difficulty scaling.
         */
        memset(&guard, 0, sizeof(guard));
        guard.strategy = 0;
        guard.unknown_17 = 1;
        guard.unknown_18 = 0;
        rng = 1;
        expected_rng = rng;
        (void)N3D_RE_RngNext(&expected_rng);
        const uint16_t timer_rand =
            N3D_RE_RngNext(&expected_rng);

        assert(N3D_RE_PlanStrategy0MovementWithRng(
            &guard, &object,
            64, 64,
            1, &rng));
        assert(rng == expected_rng);
        assert(guard.timer ==
               (int16_t)(timer_rand % 8u + 8u));

        memset(&guard, 0, sizeof(guard));
        guard.strategy = 0;
        guard.unknown_17 = 1;
        rng = 1;
        assert(N3D_RE_PlanStrategy0MovementWithRng(
            &guard, &object, 64, 64, 0, &rng));
        assert(guard.timer >= 16 && guard.timer <= 30);

        memset(&guard, 0, sizeof(guard));
        guard.strategy = 0;
        guard.unknown_17 = 1;
        rng = 1;
        assert(N3D_RE_PlanStrategy0MovementWithRng(
            &guard, &object, 64, 64, 2, &rng));
        assert(guard.timer >= 4 && guard.timer <= 7);

        /* Proximity cache forces timer 8 and suppresses second timer RNG. */
        memset(&guard, 0, sizeof(guard));
        guard.strategy = 0;
        guard.unknown_17 = 1;
        guard.unknown_18 = 1;
        rng = 1;
        expected_rng = rng;
        (void)N3D_RE_RngNext(&expected_rng);
        assert(N3D_RE_PlanStrategy0MovementWithRng(
            &guard, &object, 64, 64, 1, &rng));
        assert(rng == expected_rng);
        assert(guard.timer == 8);

        /*
         * Strategy 1 low-strength FLEE consumes no RNG. With no door it keeps
         * the old vector; with a door it aims 32 units inside the target.
         */
        memset(&guard, 0, sizeof(guard));
        guard.strategy = 1;
        guard.strength = 126;
        guard.move_x = 0;
        guard.move_y = -8;
        rng = 1234;
        expected_rng = rng;

        assert(N3D_RE_PlanStrategy1MovementWithRng(
            &guard, &object,
            0, 0, 1, &rng,
            0, 0, 0));
        assert(rng == expected_rng);
        assert(guard.move_x == 0 && guard.move_y == -8);
        assert(guard.timer == 0x10);
        assert(guard.state == N3D_GUARD_STATE_06);
        assert(guard.octant == 0);

        memset(&guard, 0, sizeof(guard));
        guard.strategy = 1;
        guard.strength = 126;
        rng = 1234;
        assert(N3D_RE_PlanStrategy1MovementWithRng(
            &guard, &object,
            0, 0, 1, &rng,
            1, 192, 64));
        assert(rng == 1234);
        assert(guard.move_x == 8);
        assert(guard.move_y == -8);
        assert(guard.timer == 0x10);
        assert(guard.octant == 1);

        /* Strength >=127 reuses pursuit and therefore consumes RNG. */
        memset(&guard, 0, sizeof(guard));
        guard.strategy = 1;
        guard.strength = 127;
        guard.unknown_17 = 1;
        guard.unknown_18 = 1;
        rng = 1;
        expected_rng = rng;
        (void)N3D_RE_RngNext(&expected_rng);
        assert(N3D_RE_PlanStrategy1MovementWithRng(
            &guard, &object,
            64, 64, 1, &rng,
            0, 0, 0));
        assert(rng == expected_rng);
        assert(guard.timer == 8);

        /* Strategy 2 consumes exactly one RNG for timer. */
        memset(&guard, 0, sizeof(guard));
        guard.strategy = 2;
        guard.move_x = -8;
        guard.move_y = 0;
        rng = 1;
        expected_rng = rng;
        const uint16_t strategy2_rand =
            N3D_RE_RngNext(&expected_rng);

        assert(N3D_RE_PlanStrategy2MovementWithRng(
            &guard, &rng));
        assert(rng == expected_rng);
        assert(guard.timer ==
               (int16_t)(strategy2_rand % 8u + 8u));
        assert(guard.state == N3D_GUARD_STATE_06);
        assert(guard.octant == 6);

        /* Strategy 3+ keeps timer/vector and consumes no RNG. */
        memset(&guard, 0, sizeof(guard));
        guard.strategy = 3;
        guard.timer = 23;
        guard.move_x = 8;
        guard.move_y = -8;
        rng = 9876;

        assert(N3D_RE_PlanMovement76FCWithRng(
            &guard, &object,
            0, 0, 1, &rng,
            0, 0, 0));
        assert(rng == 9876);
        assert(guard.timer == 23);
        assert(guard.move_x == 8 && guard.move_y == -8);
        assert(guard.state == N3D_GUARD_STATE_06);
        assert(guard.octant == 1);
    }

    /* Exact GUARD -> player distance/class/difficulty damage producer. */
    {
        assert(N3D_RE_OriginalRoundedSqrt(-1) == 0);
        assert(N3D_RE_OriginalRoundedSqrt(0) == 0);
        assert(N3D_RE_OriginalRoundedSqrt(1) == 1);
        assert(N3D_RE_OriginalRoundedSqrt(2) == 2);
        assert(N3D_RE_OriginalRoundedSqrt(4) == 2);
        assert(N3D_RE_OriginalRoundedSqrt(5) == 3);
        assert(N3D_RE_OriginalRoundedSqrt(8) == 3);
        assert(N3D_RE_OriginalRoundedSqrt(9) == 3);

        assert(N3D_RE_ComputeGuardAttackDistanceMetric(
            10 * 64 + 32, 10 * 64 + 32,
            13 * 64 + 32, 14 * 64 + 32) == 5);

        assert(N3D_RE_GuardAttackDamageUsesRandom(0x08));
        assert(N3D_RE_GuardAttackDamageUsesRandom(0x09));
        assert(N3D_RE_GuardAttackDamageUsesRandom(0x0A));
        assert(N3D_RE_GuardAttackDamageUsesRandom(0x11));
        assert(N3D_RE_GuardAttackDamageUsesRandom(0x12));
        assert(N3D_RE_GuardAttackDamageUsesRandom(0x13));
        assert(N3D_RE_GuardAttackDamageUsesRandom(0x14));
        assert(!N3D_RE_GuardAttackDamageUsesRandom(0x0B));
        assert(!N3D_RE_GuardAttackDamageUsesRandom(0x16));

        assert(N3D_RE_ApplyGuardAttackClassTransform(
            50, 0x08, 0, 0x1234) == (0x1234 & 0x07));
        assert(N3D_RE_ApplyGuardAttackClassTransform(
            50, 0x09, 0, 0x1234) == (0x1234 & 0x0F));
        assert(N3D_RE_ApplyGuardAttackClassTransform(
            100, 0x0B, 0, 0) == 25);
        assert(N3D_RE_ApplyGuardAttackClassTransform(
            40, 0x0C, 0, 0) == 40);
        assert(N3D_RE_ApplyGuardAttackClassTransform(
            50, 0x11, 0, 0x1234) == (0x1234 & 0x1F));
        assert(N3D_RE_ApplyGuardAttackClassTransform(
            50, 0x16, 0, 0) == 0x21);
        assert(N3D_RE_ApplyGuardAttackClassTransform(
            50, 0x16, 1, 0) == 100);
        assert(N3D_RE_ApplyGuardAttackClassTransform(
            50, 0x19, 0, 0) == 100);
        assert(N3D_RE_ApplyGuardAttackClassTransform(
            50, 0x18, 0, 0) == 25);

        n3d_guard_attack_damage_result attack_damage =
            N3D_RE_ComputeGuardToPlayerDamage(
                5, 0x0C, 1, 0, 0);
        assert(attack_damage.distance_seed == 20);
        assert(attack_damage.class_transformed == 20);
        assert(attack_damage.difficulty_transformed == 20);
        assert(attack_damage.stored_byte == 20);

        attack_damage =
            N3D_RE_ComputeGuardToPlayerDamage(
                5, 0x0C, 0, 0, 0);
        assert(attack_damage.difficulty_transformed == 10);

        attack_damage =
            N3D_RE_ComputeGuardToPlayerDamage(
                5, 0x0C, 2, 0, 0);
        assert(attack_damage.difficulty_transformed == 40);

        attack_damage =
            N3D_RE_ComputeGuardToPlayerDamageFromWorld(
                10 * 64 + 32, 10 * 64 + 32,
                13 * 64 + 32, 14 * 64 + 32,
                0x0C, 1, 0, 0);
        assert(attack_damage.distance_seed == 20);
        assert(attack_damage.difficulty_transformed == 20);

        attack_damage =
            N3D_RE_ComputeGuardToPlayerDamage(
                1, 0x19, 2, 0, 0);
        assert(attack_damage.class_transformed == 100);
        assert(attack_damage.difficulty_transformed == 200);
        assert(attack_damage.stored_byte == 200);
    }

    /* Class-0x16 GUARD->player full-damage gate runtime override. */
    {
        N3D_RE_ResetGuardAttackRuntime();
        assert(!N3D_RE_GuardAttackClass16FullDamageGate(1));
        assert(!N3D_RE_GuardAttackClass16FullDamageGate(2));
        assert(N3D_RE_GuardAttackClass16FullDamageGate(3));

        N3D_RE_SetGuardAttackClass16FullDamageOverride(1);
        assert(N3D_RE_GuardAttackClass16FullDamageGate(1));
        assert(N3D_RE_GuardAttackClass16FullDamageGate(2));

        N3D_RE_ResetRuntime();
        assert(!N3D_RE_GuardAttackClass16FullDamageGate(1));
        assert(N3D_RE_GuardAttackClass16FullDamageGate(3));
    }

    puts("C-rewrite recovered runtime self-test: PASS");
    return 0;
}
