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
    pickup_payload[pickup_deferred_cell * N3D_MAP_CELL_BYTES + 1] = 0x12;

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
    assert(n3d_player.silver_ammo == 119);

    pickup_result = N3D_RE_ApplyPickupAtCell(5, 5);
    assert(pickup_result.kind == N3D_PICKUP_AMMO_AT_THRESHOLD);
    assert(pickup_result.ammo_pool == N3D_AMMO_POOL_LASER);
    assert(pickup_result.value_before == 100);
    assert(pickup_result.value_after == 100);
    assert(n3d_player.laser_ammo == 100);

    pickup_result = N3D_RE_ApplyPickupAtCell(6, 6);
    assert(pickup_result.kind == N3D_PICKUP_AMMO_ADDED);
    assert(pickup_result.ammo_pool == N3D_AMMO_POOL_WAND);
    assert(pickup_result.value_before == 90);
    assert(pickup_result.value_after == 110);
    assert(n3d_player.wand_ammo == 110);

    pickup_result = N3D_RE_ApplyPickupAtCell(7, 7);
    assert(pickup_result.kind == N3D_PICKUP_DEFERRED);
    assert(pickup_result.object_class == 0x33);

    assert(N3D_RE_ApplyPickupAtCell(63, 63).kind ==
           N3D_PICKUP_UNRESOLVED);

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

    puts("C-rewrite recovered runtime self-test: PASS");
    return 0;
}
