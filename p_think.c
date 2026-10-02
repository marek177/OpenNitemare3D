#include "p_think.h"

obj_t* p_player;
int p_use_rising_edge;

void P_InitPlayer(obj_t* player)
{
    if(!player)
        return;

    player->player = calloc(1, sizeof(player_t));
    player->player->state = PLAYERSTATE_ALIVE;
    p_player = player;
}

void P_PlayerHandleInput()
{
    uint16_t mask = 0;

    /*
     * Physical SDL keys remain a port choice. The bit values below are the
     * recovered NITE3W semantic input mask used by gameplay/demo logic.
     */
    if(I_IsKeyDown(SDL_SCANCODE_UP))      mask |= N3D_INPUT_FORWARD;
    if(I_IsKeyDown(SDL_SCANCODE_DOWN))    mask |= N3D_INPUT_BACKWARD;
    if(I_IsKeyDown(SDL_SCANCODE_LEFT))    mask |= N3D_INPUT_TURN_A;
    if(I_IsKeyDown(SDL_SCANCODE_RIGHT))   mask |= N3D_INPUT_TURN_B;
    if(I_IsKeyDown(SDL_SCANCODE_LCTRL))   mask |= N3D_INPUT_FIRE;
    if(I_IsKeyDown(SDL_SCANCODE_SPACE))   mask |= N3D_INPUT_USE;

    n3d_player.input_mask = mask;
    p_use_rising_edge =
        N3D_RE_UseRisingEdge((mask & N3D_INPUT_USE) != 0);
}

void P_PlayerThink()
{
    if(!p_player || !p_player->player)
        return;

    P_PlayerHandleInput();

    player_t* player = p_player->player;

    /*
     * Historical movement remains here temporarily. Its trigonometry and
     * collision are not original NITE3W behavior and will be replaced only
     * after the exact 8604 axis-step order is closed.
     */
    if(N3D_RE_HasInput(N3D_INPUT_FORWARD))
    {
        p_player->velx = sin(player->angle) * player->speed;
        p_player->vely = sin(player->angle) * player->speed;
    }
}
