#ifndef N3D_RE_PLAYER_H
#define N3D_RE_PLAYER_H

#include "n3d_re_collision.h"

#include <stdint.h>

#define N3D_PLAYER_MAX_HEALTH 100
#define N3D_NORMAL_AMMO_CAP 100
#define N3D_SIGNED_AMMO_SAFE_MAX 127
#define N3D_AMMO_PICKUP_AMOUNT 20
#define N3D_AMMO_FORCED_SET_AMOUNT 50
#define N3D_ALL_PENTAGRAMS_MASK 0x0F

enum n3d_input_mask
{
    N3D_INPUT_ESCAPE   = 0x0001,
    N3D_INPUT_FORWARD  = 0x0002,
    N3D_INPUT_BACKWARD = 0x0004,
    N3D_INPUT_TURN_A   = 0x0008,
    N3D_INPUT_TURN_B   = 0x0010,
    N3D_INPUT_FAST     = 0x0020,
    N3D_INPUT_FINE     = 0x0040,
    N3D_INPUT_FIRE     = 0x0080,
    N3D_INPUT_STRAFE   = 0x0100,
    N3D_INPUT_USE      = 0x0200
};

typedef enum n3d_player_weapon
{
    N3D_WEAPON_SINGLE_LASER = 0,
    N3D_WEAPON_MAGIC_WAND = 1,
    N3D_WEAPON_SILVER_PISTOL = 2,
    N3D_WEAPON_CONTINUOUS_LASER = 3,
    N3D_WEAPON_NONE = 0xFF
} n3d_player_weapon;

typedef enum n3d_player_damage_result
{
    N3D_PLAYER_DAMAGE_SUPPRESSED_OMNIPOTENT,
    N3D_PLAYER_DAMAGE_SUPPRESSED_STATE2,
    N3D_PLAYER_DAMAGE_NONLETHAL,
    N3D_PLAYER_DAMAGE_LETHAL
} n3d_player_damage_result;

typedef struct n3d_player_runtime
{
    int16_t world_x;
    int16_t world_y;
    int16_t tile_x;
    int16_t tile_y;
    uint16_t map_cell_offset;

    uint8_t health;
    uint16_t game_state;
    uint8_t omnipotent;
    uint8_t difficulty;

    uint8_t silver_ammo;
    uint8_t laser_ammo;
    uint8_t wand_ammo;
    uint8_t active_weapon;
    uint8_t weapon_jam;

    uint8_t colored_keys;
    uint8_t id_cards;
    uint8_t pentagrams;

    uint16_t input_mask;
} n3d_player_runtime;

extern n3d_player_runtime n3d_player;

void N3D_RE_ResetPlayer(void);
void N3D_RE_InitPlayerAtTile(uint8_t tile_x, uint8_t tile_y);
uint8_t N3D_RE_ClampPlayerHealthForHud(void);
int N3D_RE_ApplyFixedHealthPickup(uint8_t amount);
n3d_player_damage_result N3D_RE_ApplyEnemyDamage(uint8_t damage);
int N3D_RE_CommitPlayerWorldPosition(int32_t world_x, int32_t world_y, uint8_t* event_id);

int N3D_RE_HasInventoryBit(uint8_t mask, uint8_t bit);
void N3D_RE_GrantInventoryBit(uint8_t* mask, uint8_t bit);
int N3D_RE_HasAllPentagrams(void);
int N3D_RE_HasInput(uint16_t bit);

#endif
