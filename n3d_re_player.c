#include "n3d_re_player.h"

#include <string.h>

n3d_player_runtime n3d_player;

void N3D_RE_ResetPlayer(void)
{
    memset(&n3d_player, 0, sizeof(n3d_player));
    n3d_player.health = N3D_PLAYER_MAX_HEALTH;
    n3d_player.active_weapon = N3D_WEAPON_NONE;
}

void N3D_RE_InitPlayerAtTile(uint8_t tile_x, uint8_t tile_y)
{
    N3D_RE_ResetPlayer();

    n3d_player.tile_x = tile_x;
    n3d_player.tile_y = tile_y;
    n3d_player.world_x =
        (int16_t)(tile_x * N3D_WORLD_UNITS_PER_TILE + N3D_TILE_CENTER_OFFSET);
    n3d_player.world_y =
        (int16_t)(tile_y * N3D_WORLD_UNITS_PER_TILE + N3D_TILE_CENTER_OFFSET);
    n3d_player.map_cell_offset =
        (uint16_t)((tile_y * N3D_MAP_WIDTH + tile_x) * N3D_MAP_CELL_BYTES);
}

uint8_t N3D_RE_ClampPlayerHealthForHud(void)
{
    if (n3d_player.health > N3D_PLAYER_MAX_HEALTH)
        n3d_player.health = N3D_PLAYER_MAX_HEALTH;

    return n3d_player.health;
}

int N3D_RE_ApplyFixedHealthPickup(uint8_t amount)
{
    if (amount != 20 && amount != 30)
        return 0;

    if (n3d_player.health >= N3D_PLAYER_MAX_HEALTH)
        return 0;

    n3d_player.health = (uint8_t)(n3d_player.health + amount);
    return 1;
}

n3d_player_damage_result N3D_RE_ApplyEnemyDamage(uint8_t damage)
{
    if (n3d_player.omnipotent)
        return N3D_PLAYER_DAMAGE_SUPPRESSED_OMNIPOTENT;

    if (n3d_player.game_state == 2)
        return N3D_PLAYER_DAMAGE_SUPPRESSED_STATE2;

    if (damage >= n3d_player.health)
    {
        n3d_player.health = 0;
        n3d_player.game_state = 2;
        return N3D_PLAYER_DAMAGE_LETHAL;
    }

    n3d_player.health = (uint8_t)(n3d_player.health - damage);
    return N3D_PLAYER_DAMAGE_NONLETHAL;
}

int N3D_RE_CommitPlayerWorldPosition(
    int32_t world_x,
    int32_t world_y,
    uint8_t* event_id)
{
    n3d_post_move_result post = N3D_RE_PostMoveCell(
        world_x,
        world_y,
        n3d_player.tile_x,
        n3d_player.tile_y);

    if (!post.valid ||
        world_x < INT16_MIN || world_x > INT16_MAX ||
        world_y < INT16_MIN || world_y > INT16_MAX)
        return 0;

    n3d_player.world_x = (int16_t)world_x;
    n3d_player.world_y = (int16_t)world_y;
    n3d_player.tile_x = (int16_t)post.tile_x;
    n3d_player.tile_y = (int16_t)post.tile_y;
    n3d_player.map_cell_offset = post.map_byte_offset;

    if (event_id)
        *event_id = post.entered_tile_event;

    return 1;
}

int N3D_RE_HasInventoryBit(uint8_t mask, uint8_t bit)
{
    if (bit >= 8)
        return 0;

    return (mask & (uint8_t)(1u << bit)) != 0;
}

void N3D_RE_GrantInventoryBit(uint8_t* mask, uint8_t bit)
{
    if (!mask || bit >= 8)
        return;

    *mask = (uint8_t)(*mask | (uint8_t)(1u << bit));
}

int N3D_RE_HasAllPentagrams(void)
{
    return (n3d_player.pentagrams & N3D_ALL_PENTAGRAMS_MASK) ==
           N3D_ALL_PENTAGRAMS_MASK;
}

int N3D_RE_HasInput(uint16_t bit)
{
    return (n3d_player.input_mask & bit) != 0;
}
