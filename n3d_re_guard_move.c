#include "n3d_re_guard_move.h"

#include "n3d_re_guard_plan.h"

#include <string.h>

static int N3D_RE_AbsMove(int value)
{
    return value < 0 ? -value : value;
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

    int8_t step =
        (int8_t)guard->unknown_15;

    if(step == 0)
        step = 1;

    object->runtime_1a =
        (uint8_t)(
            (int)object->runtime_1a +
            (int)step);

    if((int8_t)object->runtime_1a <= 10)
    {
        object->runtime_1a = 10;
        step = (int8_t)-step;
    }

    if(object->runtime_1a >= 0x23)
    {
        object->runtime_1a = 0x23;
        step = (int8_t)-step;
    }

    guard->unknown_15 =
        (uint8_t)step;
    return 1;
}

int N3D_RE_GuardMovementCandidateTouchesPlayer(
    int16_t candidate_x,
    int16_t candidate_y,
    int16_t player_world_x,
    int16_t player_world_y)
{
    return
        N3D_RE_AbsMove(
            (int)candidate_x -
            player_world_x) < 0x2A &&
        N3D_RE_AbsMove(
            (int)candidate_y -
            player_world_y) < 0x2A;
}

int16_t N3D_RE_GuardDirectionPadding(int8_t component)
{
    if(component < 0)
        return -0x10;
    if(component > 0)
        return 0x10;
    return 0;
}

void N3D_RE_AdvanceGuardMovementFrame(
    n3d_guard_record* guard,
    n3d_object_record* object)
{
    if(!guard || !object)
        return;

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
    n3d_guard_world_block_callback is_blocked_at,
    void* user,
    uint32_t* rng_state)
{
    n3d_guard_movement_result result;
    memset(&result, 0, sizeof(result));

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

    /*
     * State 8 refuses the whole coordinate commit when either axis blocks.
     * State 6 and other movement states slide along the unblocked axis.
     */
    const int position_committed =
        guard->state != N3D_GUARD_STATE_08 ||
        (!x_blocked && !y_blocked);

    if(position_committed)
    {
        object->world_x =
            (int16_t)(
                object->world_x +
                applied_x);
        object->world_y =
            (int16_t)(
                object->world_y +
                applied_y);
    }

    int bounced = 0;
    int octant_x = applied_x;
    int octant_y = applied_y;

    if(guard->state == N3D_GUARD_STATE_06 &&
       x_blocked &&
       y_blocked)
    {
        bounced = 1;

        const uint16_t bounce_random =
            rng_state
                ? N3D_RE_RngNext(rng_state)
                : 0;

        /*
         * Win16 compiler-visible quirk: the negated signed byte is copied
         * only as its LOW BYTE into a zeroed 16-bit local before FUN_6E66.
         * Thus -8 becomes +248 for the octant helper on the bounced axis.
         */
        if(bounce_random & 1u)
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

    result.x_blocked =
        x_blocked ? 1 : 0;
    result.y_blocked =
        y_blocked ? 1 : 0;
    result.position_committed =
        position_committed ? 1 : 0;
    result.bounced =
        bounced ? 1 : 0;
    result.applied_x =
        applied_x;
    result.applied_y =
        applied_y;

    return result;
}

n3d_guard_movement_result
N3D_RE_TickGuardMovementCollisionCore(
    n3d_guard_record* guard,
    n3d_object_record* object,
    n3d_guard_world_block_callback is_blocked_at,
    void* user)
{
    return N3D_RE_TickGuardMovementCollisionCoreWithRng(
        guard,
        object,
        is_blocked_at,
        user,
        &n3d_original_rng_state);
}
