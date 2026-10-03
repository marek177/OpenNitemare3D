#include "n3d_re_guard_move.h"

#include "n3d_re_guard_plan.h"

static int N3D_RE_AbsInt(int value)
{
    return value < 0 ? -value : value;
}

uint8_t N3D_RE_ComputeGuardPlayerOctant(
    uint8_t control,
    int16_t guard_world_x,
    int16_t guard_world_y,
    int16_t player_world_x,
    int16_t player_world_y)
{
    const int delta_x =
        (int)player_world_x - guard_world_x;
    const int delta_y =
        (int)player_world_y - guard_world_y;
    const int abs_x = N3D_RE_AbsInt(delta_x);
    const int abs_y = N3D_RE_AbsInt(delta_y);

    if(control != 0)
    {
        if(abs_y * 2 < abs_x)
            return delta_x > 0 ? 2 : 6;

        if(abs_x * 2 < abs_y)
            return delta_y > 0 ? 4 : 0;

        if(delta_x > 0)
            return delta_y > 0 ? 3 : 1;

        return delta_y > 0 ? 5 : 7;
    }

    if(delta_x < 0)
    {
        if(delta_y < 0)
            return abs_y > abs_x ? 7 : 6;

        return abs_y > abs_x ? 4 : 5;
    }

    if(delta_y < 0)
        return abs_y > abs_x ? 0 : 1;

    return abs_y <= abs_x ? 2 : 3;
}

uint8_t N3D_RE_ComputeGuardResultOctant(
    const n3d_guard_record* guard,
    const n3d_object_record* object,
    int16_t player_world_x,
    int16_t player_world_y)
{
    if(!guard || !object)
        return 0;

    const uint8_t player_octant =
        N3D_RE_ComputeGuardPlayerOctant(
            guard->control,
            object->world_x,
            object->world_y,
            player_world_x,
            player_world_y);

    const int base_offset =
        guard->control == 0 ? 3 : 4;

    return (uint8_t)(
        (base_offset + guard->octant - player_octant) & 7);
}

uint16_t N3D_RE_GetGuardDirectionalSequence(
    const n3d_object_definition_header* definition,
    uint8_t state,
    uint8_t strategy,
    uint8_t result_octant)
{
    if(!definition)
        return 0;

    unsigned bank = 0;

    if(state == N3D_GUARD_STATE_06 &&
       strategy != 2)
    {
        bank = 2;
    }
    else if(state == N3D_GUARD_STATE_08 ||
            state == N3D_GUARD_STATE_10 ||
            state == N3D_GUARD_STATE_11)
    {
        bank = 1;
    }

    return N3D_RE_ObjectDefinitionDirectional(
        definition,
        bank,
        result_octant & 7u);
}

n3d_guard_dispatch_result N3D_RE_RefreshGuardDirectionalSequence(
    n3d_guard_record* guard,
    n3d_object_record* object,
    const n3d_object_definition_header* definition,
    int16_t player_world_x,
    int16_t player_world_y,
    int force_refresh)
{
    if(!guard || !object || !definition)
        return N3D_GUARD_DISPATCH_NOT_HANDLED;

    const uint8_t result_octant =
        N3D_RE_ComputeGuardResultOctant(
            guard,
            object,
            player_world_x,
            player_world_y);

    if(!force_refresh &&
       guard->result_octant == result_octant)
    {
        return N3D_GUARD_DISPATCH_WAITING;
    }

    guard->result_octant = result_octant;

    const uint16_t sequence =
        N3D_RE_GetGuardDirectionalSequence(
            definition,
            guard->state,
            guard->strategy,
            result_octant);

    guard->definition_value = sequence;
    object->animation_frame =
        (int8_t)(uint8_t)sequence;

    return N3D_GUARD_DISPATCH_TRANSITIONED;
}

int N3D_RE_TickGuardVerticalBob(
    n3d_guard_record* guard,
    n3d_object_record* object)
{
    if(!guard || !object)
        return 0;

    if(object->object_class != 0x08 &&
       object->object_class != 0x14 &&
       object->object_class != 0x1A)
    {
        return 0;
    }

    int8_t bob_step =
        (int8_t)guard->unknown_15;

    if(bob_step == 0)
        bob_step = 1;

    object->runtime_1a =
        (uint8_t)(
            (int)object->runtime_1a +
            bob_step);

    if((int8_t)object->runtime_1a <= 10)
    {
        object->runtime_1a = 10;
        bob_step = (int8_t)-bob_step;
    }

    if(object->runtime_1a >= 0x23)
    {
        object->runtime_1a = 0x23;
        bob_step = (int8_t)-bob_step;
    }

    guard->unknown_15 =
        (uint8_t)bob_step;

    return 1;
}

int N3D_RE_GuardMovementCandidateTouchesPlayer(
    int16_t candidate_world_x,
    int16_t candidate_world_y,
    int16_t player_world_x,
    int16_t player_world_y)
{
    return
        N3D_RE_AbsInt(
            (int)candidate_world_x -
            player_world_x) < 0x2A &&
        N3D_RE_AbsInt(
            (int)candidate_world_y -
            player_world_y) < 0x2A;
}

int16_t N3D_RE_GuardDirectionPadding(
    int8_t component)
{
    if(component < 0)
        return -0x10;
    if(component > 0)
        return 0x10;
    return 0;
}

static void N3D_RE_AdvanceGuardMovementFrame(
    n3d_guard_record* guard,
    n3d_object_record* object)
{
    const int first_frame =
        guard->definition_value & 0xFF;
    const int frame_count =
        (guard->definition_value >> 8) & 0xFF;
    int frame =
        ((uint8_t)object->animation_frame + 1) & 0xFF;

    if(first_frame + frame_count <= frame)
        frame = first_frame;

    object->animation_frame =
        (int8_t)(uint8_t)frame;
}

n3d_guard_movement_result
N3D_RE_TickGuardMovementCollisionCoreWithRng(
    n3d_guard_record* guard,
    n3d_object_record* object,
    n3d_guard_block_callback is_blocked_at,
    void* user,
    uint32_t* rng_state)
{
    n3d_guard_movement_result result = {0};

    if(!guard || !object || !is_blocked_at)
        return result;

    N3D_RE_TickGuardVerticalBob(
        guard,
        object);

    const int8_t original_move_x =
        guard->move_x;
    const int8_t original_move_y =
        guard->move_y;

    const int16_t x_probe =
        (int16_t)(
            object->world_x +
            original_move_x +
            N3D_RE_GuardDirectionPadding(
                original_move_x));

    const int16_t y_probe =
        (int16_t)(
            object->world_y +
            original_move_y +
            N3D_RE_GuardDirectionPadding(
                original_move_y));

    const int x_blocked =
        original_move_x != 0 &&
        (is_blocked_at(
             x_probe,
             (int16_t)(object->world_y - 0x10),
             user) ||
         is_blocked_at(
             x_probe,
             (int16_t)(object->world_y + 0x10),
             user));

    const int y_blocked =
        original_move_y != 0 &&
        (is_blocked_at(
             (int16_t)(object->world_x - 0x10),
             y_probe,
             user) ||
         is_blocked_at(
             (int16_t)(object->world_x + 0x10),
             y_probe,
             user));

    const int16_t applied_x =
        x_blocked ? 0 : original_move_x;
    const int16_t applied_y =
        y_blocked ? 0 : original_move_y;

    const int position_committed =
        guard->state != N3D_GUARD_STATE_08 ||
        (!x_blocked && !y_blocked);

    if(position_committed)
    {
        object->world_x =
            (int16_t)(object->world_x + applied_x);
        object->world_y =
            (int16_t)(object->world_y + applied_y);
    }

    int octant_x = applied_x;
    int octant_y = applied_y;
    int bounced = 0;

    if(guard->state == N3D_GUARD_STATE_06 &&
       x_blocked &&
       y_blocked)
    {
        bounced = 1;

        const uint16_t bounce_random =
            rng_state
                ? N3D_RE_RngNext(rng_state)
                : 0;

        if((bounce_random & 1u) != 0)
        {
            guard->move_x =
                (int8_t)-guard->move_x;
            octant_x =
                (uint8_t)guard->move_x;
            octant_y = 0;
        }
        else
        {
            guard->move_y =
                (int8_t)-guard->move_y;
            octant_x = 0;
            octant_y =
                (uint8_t)guard->move_y;
        }
    }

    if(octant_x != 0 || octant_y != 0)
    {
        N3D_RE_AdvanceGuardMovementFrame(
            guard,
            object);
    }

    N3D_RE_UpdateGuardOctantFromMovement(
        guard,
        octant_x,
        octant_y);

    result.x_blocked = x_blocked ? 1 : 0;
    result.y_blocked = y_blocked ? 1 : 0;
    result.position_committed =
        position_committed ? 1 : 0;
    result.bounced = bounced ? 1 : 0;
    result.applied_x = applied_x;
    result.applied_y = applied_y;

    return result;
}

n3d_guard_movement_result N3D_RE_TickGuardMovementCollisionCore(
    n3d_guard_record* guard,
    n3d_object_record* object,
    n3d_guard_block_callback is_blocked_at,
    void* user)
{
    return N3D_RE_TickGuardMovementCollisionCoreWithRng(
        guard,
        object,
        is_blocked_at,
        user,
        &n3d_original_rng_state);
}

n3d_guard_dispatch_result N3D_RE_TickState06Movement(
    n3d_guard_record* guard,
    n3d_object_record* object,
    const n3d_object_definition_header* definition,
    int16_t player_world_x,
    int16_t player_world_y,
    n3d_guard_block_callback is_blocked_at,
    void* user,
    uint32_t* rng_state,
    n3d_guard_movement_result* out_movement)
{
    if(out_movement)
    {
        n3d_guard_movement_result empty = {0};
        *out_movement = empty;
    }

    if(!guard ||
       !object ||
       !definition ||
       guard->state != N3D_GUARD_STATE_06)
    {
        return N3D_GUARD_DISPATCH_NOT_HANDLED;
    }

    N3D_RE_RefreshGuardDirectionalSequence(
        guard,
        object,
        definition,
        player_world_x,
        player_world_y,
        0);

    const n3d_guard_movement_result movement =
        N3D_RE_TickGuardMovementCollisionCoreWithRng(
            guard,
            object,
            is_blocked_at,
            user,
            rng_state);

    if(out_movement)
        *out_movement = movement;

    --guard->timer;

    if(guard->timer == 0)
    {
        guard->state =
            N3D_GUARD_STATE_03;
        return N3D_GUARD_DISPATCH_TRANSITIONED;
    }

    if(movement.applied_x != 0 ||
       movement.applied_y != 0)
    {
        return movement.position_committed
            ? N3D_GUARD_DISPATCH_MOVED
            : N3D_GUARD_DISPATCH_MOVEMENT_BLOCKED;
    }

    return movement.x_blocked ||
           movement.y_blocked
        ? N3D_GUARD_DISPATCH_MOVEMENT_BLOCKED
        : N3D_GUARD_DISPATCH_WAITING;
}
