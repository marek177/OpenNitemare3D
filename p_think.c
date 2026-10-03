#include "p_think.h"

obj_t* p_player;
int p_use_rising_edge;
n3d_use_execution p_last_use_execution;

static void P_SyncLegacyPlayerWrapper(void)
{
    if(!p_player || !p_player->player)
        return;

    p_player->x =
        (float)n3d_player.world_x /
        (float)N3D_WORLD_UNITS_PER_TILE;
    p_player->y =
        (float)n3d_player.world_y /
        (float)N3D_WORLD_UNITS_PER_TILE;

    /* Legacy float fields remain presentation/compatibility mirrors only. */
    p_player->velx = 0;
    p_player->vely = 0;
    p_player->health = n3d_player.health;
    p_player->player->angle = (float)n3d_player.angle_degrees;
}

void P_InitPlayer(obj_t* player)
{
    if(!player)
        return;

    if(!player->player)
        player->player = calloc(1, sizeof(player_t));

    if(!player->player)
        return;

    player->player->state = PLAYERSTATE_ALIVE;
    p_player = player;
    P_SyncLegacyPlayerWrapper();
}

void P_PlayerHandleInput()
{
    uint16_t mask = 0;

    /*
     * Physical SDL bindings are the port frontend; these bit values are the
     * recovered NITE3W gameplay/demo input mask.
     */
    if(I_IsKeyDown(SDL_SCANCODE_ESCAPE))   mask |= N3D_INPUT_ESCAPE;
    if(I_IsKeyDown(SDL_SCANCODE_UP))       mask |= N3D_INPUT_FORWARD;
    if(I_IsKeyDown(SDL_SCANCODE_DOWN))     mask |= N3D_INPUT_BACKWARD;
    if(I_IsKeyDown(SDL_SCANCODE_LEFT))     mask |= N3D_INPUT_TURN_A;
    if(I_IsKeyDown(SDL_SCANCODE_RIGHT))    mask |= N3D_INPUT_TURN_B;

    /* Original 9806: Right Shift doubles move+turn, Left Shift forces both to 1. */
    if(I_IsKeyDown(SDL_SCANCODE_RSHIFT))   mask |= N3D_INPUT_FAST;
    if(I_IsKeyDown(SDL_SCANCODE_LSHIFT))   mask |= N3D_INPUT_FINE;

    if(I_IsKeyDown(SDL_SCANCODE_LCTRL) ||
       I_IsKeyDown(SDL_SCANCODE_RCTRL))    mask |= N3D_INPUT_FIRE;

    /* Alt changes left/right from turning to +/-90 degree strafing. */
    if(I_IsKeyDown(SDL_SCANCODE_LALT) ||
       I_IsKeyDown(SDL_SCANCODE_RALT))     mask |= N3D_INPUT_STRAFE;

    if(I_IsKeyDown(SDL_SCANCODE_SPACE))    mask |= N3D_INPUT_USE;

    n3d_player.input_mask = mask;
    p_use_rising_edge =
        N3D_RE_UseRisingEdge((mask & N3D_INPUT_USE) != 0);
}

static n3d_collision_callbacks P_PlayerCollisionCallbacks(void)
{
    n3d_collision_callbacks callbacks = {
        N3D_RE_DoorCellPassableCallback,
        NULL,
        N3D_RE_PlayerPickupTouchCallback,
        NULL
    };
    return callbacks;
}

static void P_MoveAtAngle(int angle, uint16_t movement_substeps)
{
    if(movement_substeps == 0 || !n3d_trig.loaded)
        return;

    n3d_collision_callbacks callbacks =
        P_PlayerCollisionCallbacks();

    N3D_RE_MovePlayerAngleSubsteps(
        angle,
        movement_substeps,
        &callbacks);
}

void P_PlayerThink()
{
    if(!p_player || !p_player->player)
        return;

    P_PlayerHandleInput();

    const n3d_control_steps control =
        N3D_RE_ControlStepsForInput(n3d_player.input_mask);

    /*
     * Exact 9806 processing order:
     * forward, backward, left, right, then later fire/use handling.
     */
    if(N3D_RE_HasInput(N3D_INPUT_FORWARD))
    {
        P_MoveAtAngle(
            N3D_RE_ForwardAngle(n3d_player.angle_degrees),
            control.movement_substeps);
    }

    if(N3D_RE_HasInput(N3D_INPUT_BACKWARD))
    {
        P_MoveAtAngle(
            N3D_RE_BackwardAngle(n3d_player.angle_degrees),
            control.movement_substeps);
    }

    if(N3D_RE_HasInput(N3D_INPUT_TURN_A))
    {
        if(N3D_RE_HasInput(N3D_INPUT_STRAFE))
        {
            P_MoveAtAngle(
                N3D_RE_StrafeLeftAngle(
                    n3d_player.angle_degrees),
                control.movement_substeps);
        }
        else
        {
            N3D_RE_TurnPlayer(
                N3D_RE_LeftTurnDelta(
                    n3d_player.input_mask));
        }
    }

    if(N3D_RE_HasInput(N3D_INPUT_TURN_B))
    {
        if(N3D_RE_HasInput(N3D_INPUT_STRAFE))
        {
            P_MoveAtAngle(
                N3D_RE_StrafeRightAngle(
                    n3d_player.angle_degrees),
                control.movement_substeps);
        }
        else
        {
            N3D_RE_TurnPlayer(
                N3D_RE_RightTurnDelta(
                    n3d_player.input_mask));
        }
    }

    if(p_use_rising_edge)
    {
        p_last_use_execution =
            N3D_RE_ExecuteUse(n3d_player.coarse_octant);
    }

    /*
     * FIRE remains a separate weapon-runtime integration target. The recovered
     * input bit is already preserved; do not route it through the old callback
     * weapons here.
     */

    P_SyncLegacyPlayerWrapper();
}
