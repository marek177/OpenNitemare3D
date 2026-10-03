#include "n3d_re_guard_plan.h"

int8_t N3D_RE_GuardSignedStepFromDelta(int delta)
{
    if(delta < 0)
        return -8;
    if(delta > 0)
        return 8;
    return 0;
}

void N3D_RE_UpdateGuardOctantFromMovement(
    n3d_guard_record* guard,
    int move_x,
    int move_y)
{
    if(!guard)
        return;

    if(move_x > 0)
    {
        guard->octant =
            move_y < 0 ? 1 :
            move_y > 0 ? 3 : 2;
        return;
    }

    if(move_x < 0)
    {
        guard->octant =
            move_y < 0 ? 7 :
            move_y > 0 ? 5 : 6;
        return;
    }

    if(move_y < 0)
        guard->octant = 0;
    else if(move_y > 0)
        guard->octant = 4;
}

int N3D_RE_PlanPlayerPursuitMovementWithRng(
    n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    uint8_t difficulty,
    uint32_t* rng_state)
{
    if(!guard || !object || !rng_state)
        return 0;

    /*
     * FUN_76FC uses signed divide-by-32. C11 signed integer division truncates
     * toward zero, matching the recovered 16-bit behavior.
     */
    const int delta_x_32 =
        ((int)player_world_x - object->world_x) / 32;
    const int delta_y_32 =
        ((int)player_world_y - object->world_y) / 32;

    const int direction_choice =
        N3D_RE_RngNext(rng_state) &
        (guard->unknown_17 == 0 ? 3 : 7);

    if(direction_choice == 0)
    {
        if(delta_x_32 == 0)
            guard->move_x = 8;
        if(delta_y_32 == 0)
            guard->move_y = 8;
    }
    else if(direction_choice == 1)
    {
        if(delta_x_32 == 0)
            guard->move_x = -8;
        if(delta_y_32 == 0)
            guard->move_y = -8;
    }
    else
    {
        guard->move_x =
            N3D_RE_GuardSignedStepFromDelta(
                delta_x_32);
        guard->move_y =
            N3D_RE_GuardSignedStepFromDelta(
                delta_y_32);
    }

    if(guard->unknown_18 != 0)
    {
        guard->timer = 8;
    }
    else if(guard->unknown_17 == 0)
    {
        guard->timer = 0x18;
    }
    else
    {
        guard->timer =
            (int16_t)(
                N3D_RE_RngNext(rng_state) % 8u + 8u);

        if(difficulty == 2)
            guard->timer =
                (int16_t)(guard->timer >> 1);
        else if(difficulty == 0)
            guard->timer =
                (int16_t)(guard->timer << 1);
    }

    guard->state = N3D_GUARD_STATE_06;
    N3D_RE_UpdateGuardOctantFromMovement(
        guard,
        guard->move_x,
        guard->move_y);

    return 1;
}

int N3D_RE_PlanStrategy0MovementWithRng(
    n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    uint8_t difficulty,
    uint32_t* rng_state)
{
    if(!guard || guard->strategy != 0)
        return 0;

    return N3D_RE_PlanPlayerPursuitMovementWithRng(
        guard,
        object,
        player_world_x,
        player_world_y,
        difficulty,
        rng_state);
}

int N3D_RE_PlanStrategy1MovementWithRng(
    n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    uint8_t difficulty,
    uint32_t* rng_state,
    int retreat_door_found,
    int16_t retreat_target_world_x,
    int16_t retreat_target_world_y)
{
    if(!guard ||
       !object ||
       !rng_state ||
       guard->strategy != 1)
        return 0;

    if(guard->strength >= 0x7F)
    {
        return N3D_RE_PlanPlayerPursuitMovementWithRng(
            guard,
            object,
            player_world_x,
            player_world_y,
            difficulty,
            rng_state);
    }

    if(retreat_door_found)
    {
        const int dx =
            (int)retreat_target_world_x -
            object->world_x + 0x20;
        const int dy =
            (int)retreat_target_world_y -
            object->world_y + 0x20;

        guard->move_x =
            N3D_RE_GuardSignedStepFromDelta(dx);
        guard->move_y =
            N3D_RE_GuardSignedStepFromDelta(dy);
    }

    /* No random value is consumed on the low-strength flee branch. */
    guard->timer = 0x10;
    guard->state = N3D_GUARD_STATE_06;

    N3D_RE_UpdateGuardOctantFromMovement(
        guard,
        guard->move_x,
        guard->move_y);

    return 1;
}

int N3D_RE_PlanStrategy2MovementWithRng(
    n3d_guard_record* guard,
    uint32_t* rng_state)
{
    if(!guard ||
       !rng_state ||
       guard->strategy != 2)
        return 0;

    guard->timer =
        (int16_t)(
            N3D_RE_RngNext(rng_state) % 8u + 8u);
    guard->state = N3D_GUARD_STATE_06;

    N3D_RE_UpdateGuardOctantFromMovement(
        guard,
        guard->move_x,
        guard->move_y);

    return 1;
}

int N3D_RE_PlanStrategyCurrentVectorMovement(
    n3d_guard_record* guard)
{
    if(!guard || guard->strategy < 3)
        return 0;

    guard->state = N3D_GUARD_STATE_06;

    N3D_RE_UpdateGuardOctantFromMovement(
        guard,
        guard->move_x,
        guard->move_y);

    return 1;
}

int N3D_RE_PlanMovement76FCWithRng(
    n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    uint8_t difficulty,
    uint32_t* rng_state,
    int retreat_door_found,
    int16_t retreat_target_world_x,
    int16_t retreat_target_world_y)
{
    if(!guard || !object || !rng_state)
        return 0;

    switch(guard->strategy)
    {
        case 0:
            return N3D_RE_PlanStrategy0MovementWithRng(
                guard,
                object,
                player_world_x,
                player_world_y,
                difficulty,
                rng_state);

        case 1:
            return N3D_RE_PlanStrategy1MovementWithRng(
                guard,
                object,
                player_world_x,
                player_world_y,
                difficulty,
                rng_state,
                retreat_door_found,
                retreat_target_world_x,
                retreat_target_world_y);

        case 2:
            return N3D_RE_PlanStrategy2MovementWithRng(
                guard,
                rng_state);

        default:
            return N3D_RE_PlanStrategyCurrentVectorMovement(
                guard);
    }
}

int N3D_RE_PlanMovement76FC(
    n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y,
    uint8_t difficulty,
    int retreat_door_found,
    int16_t retreat_target_world_x,
    int16_t retreat_target_world_y)
{
    return N3D_RE_PlanMovement76FCWithRng(
        guard,
        object,
        player_world_x,
        player_world_y,
        difficulty,
        &n3d_original_rng_state,
        retreat_door_found,
        retreat_target_world_x,
        retreat_target_world_y);
}
