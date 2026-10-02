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
    assert(!N3D_RE_KnownWallMappedTypeForClass(
        "UNRESOLVED_DOOR", &known_mapped_type));

    assert(N3D_RE_KnownObjectMappedTypeForClass(
        "PUSH", &known_mapped_type));
    assert(known_mapped_type == 0x28);
    assert(!N3D_RE_KnownObjectMappedTypeForClass(
        "UNRESOLVED_CLASS", &known_mapped_type));

    FILE* walls_test = fopen("WALLS.1", "wb");
    assert(walls_test != NULL);
    fputs("01 W WALLIMG SOLID Ordinary wall\n", walls_test);
    fputs("70 D DOORIMG UNRESOLVED_DOOR Door variant\n", walls_test);
    fputs("90 W WARPIMG WARP_1 Paired warp\n", walls_test);
    fputs("91 W KEYIMG WARP_L4 Yellow key gate\n", walls_test);
    fputs("92 W LEVELIMG LEVEL_UP Level exit\n", walls_test);
    fputs("93 W MIRRORIMG WARP_S2 Other Side mirror\n", walls_test);
    fputs("94 W TRIGIMG TRIGGER2 Trigger two\n", walls_test);
    fclose(walls_test);

    FILE* objects_test = fopen("OBJECTS.1", "wb");
    assert(objects_test != NULL);
    fputs("18 O TOMB PUSH Tombstone pushable\n", objects_test);
    fputs("80 O BATIMG UNRESOLVED_GUARD Bat north\n", objects_test);
    fclose(objects_test);

    assert(N3D_RE_LoadEpisodeDefinitions(1));
    assert(n3d_wall_definitions.count == 7);
    assert(n3d_object_definitions.count == 2);

    n3d_mapping_coverage wall_mapping_coverage =
        N3D_RE_WallMappingCoverage();
    n3d_mapping_coverage object_mapping_coverage =
        N3D_RE_ObjectMappingCoverage();
    assert(wall_mapping_coverage.total == 7);
    assert(wall_mapping_coverage.known == 5);
    assert(wall_mapping_coverage.unknown == 2);
    assert(object_mapping_coverage.total == 2);
    assert(object_mapping_coverage.known == 2);
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

    assert(!N3D_RE_WallMappingKnown(0x01));
    assert(n3d_wall_mapped_type[0x01] == N3D_MAPPED_TYPE_UNKNOWN);
    assert(!N3D_RE_WallMappingKnown(0x70));
    assert(n3d_wall_mapped_type[0x70] == N3D_MAPPED_TYPE_UNKNOWN);

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

    N3D_RE_ResetRuntime();
    n3d_map[0].wall = 0;
    n3d_map[0].object = 0;
    n3d_map[1].wall = 0;
    n3d_map[1].object = 0;

    n3d_resolved_collision_result resolved_collision =
        N3D_RE_TestResolvedLeadingEdgePair(0, 0, 1, 0, 1, NULL);
    assert(resolved_collision.resolved);
    assert(resolved_collision.step == 1);

    n3d_map[0].wall = 0x70;
    resolved_collision =
        N3D_RE_TestResolvedLeadingEdgePair(0, 0, 1, 0, 1, NULL);
    assert(!resolved_collision.resolved);
    assert(resolved_collision.step == 0);

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

    n3d_map[use_east_cell].wall = 0x70;
    use_target = N3D_RE_ClassifyUseTarget(1);
    assert(use_target.kind == N3D_USE_UNRESOLVED);

    n3d_wall_mapping_known[5] = 1;
    n3d_wall_mapped_type[5] = 0x31;
    n3d_wall_property_resolved[5] =
        N3D_RE_WallPropertiesForMappedType(0x31);
    n3d_map[use_east_cell].wall = 5;
    use_target = N3D_RE_ClassifyUseTarget(1);
    assert(use_target.kind == N3D_USE_DYNAMIC_DOOR);
    assert(use_target.mapped_wall_type == 0x31);

    n3d_map[use_east_cell].wall = 184;
    use_target = N3D_RE_ClassifyUseTarget(1);
    assert(use_target.kind == N3D_USE_MAPPED_WALL);
    assert(use_target.mapped_wall_type == 0x47);

    n3d_map[use_east_cell].wall = 0;
    n3d_object_mapping_known[98] = 1;
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

    puts("C-rewrite recovered runtime self-test: PASS");
    return 0;
}
