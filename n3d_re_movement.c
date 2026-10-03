#include "n3d_re_movement.h"

#include "n3d_re_player.h"

#include <stdlib.h>

static int N3D_RE_ProbeTile(int32_t world)
{
    return (int)N3D_RE_WorldToTile(world);
}

static int N3D_RE_TestProbePair(
    int ax, int ay,
    int bx, int by,
    int signed_step,
    const n3d_collision_callbacks* callbacks,
    int* resolved)
{
    if(resolved)
        *resolved = 0;

    if(ax < 0 || ay < 0 || bx < 0 || by < 0 ||
       ax >= N3D_MAP_WIDTH || ay >= N3D_MAP_HEIGHT ||
       bx >= N3D_MAP_WIDTH || by >= N3D_MAP_HEIGHT)
        return 0;

    const n3d_resolved_collision_result result =
        N3D_RE_TestResolvedLeadingEdgePair(
            (uint8_t)ax, (uint8_t)ay,
            (uint8_t)bx, (uint8_t)by,
            signed_step,
            callbacks);

    if(resolved)
        *resolved = result.resolved;

    return result.step;
}

int N3D_RE_TestPlayerXSubstep(
    int32_t world_x,
    int32_t world_y,
    int signed_step,
    const n3d_collision_callbacks* callbacks,
    int* resolved)
{
    if(signed_step != -1 && signed_step != 1)
    {
        if(resolved) *resolved = 0;
        return 0;
    }

    const int32_t candidate_x = world_x + signed_step;

    /*
     * Recovered geometry:
     * - leading direction probe reaches 28 units from the center;
     * - perpendicular corners use the 27-unit AABB half extent.
     */
    const int32_t leading_x =
        candidate_x + (signed_step > 0 ? 28 : -28);

    const int tile_x = N3D_RE_ProbeTile(leading_x);
    const int tile_y_a =
        N3D_RE_ProbeTile(world_y - N3D_PLAYER_COLLISION_HALF_EXTENT);
    const int tile_y_b =
        N3D_RE_ProbeTile(world_y + N3D_PLAYER_COLLISION_HALF_EXTENT);

    return N3D_RE_TestProbePair(
        tile_x, tile_y_a,
        tile_x, tile_y_b,
        signed_step,
        callbacks,
        resolved);
}

int N3D_RE_TestPlayerYSubstep(
    int32_t world_x,
    int32_t world_y,
    int signed_step,
    const n3d_collision_callbacks* callbacks,
    int* resolved)
{
    if(signed_step != -1 && signed_step != 1)
    {
        if(resolved) *resolved = 0;
        return 0;
    }

    const int32_t candidate_y = world_y + signed_step;
    const int32_t leading_y =
        candidate_y + (signed_step > 0 ? 28 : -28);

    const int tile_y = N3D_RE_ProbeTile(leading_y);
    const int tile_x_a =
        N3D_RE_ProbeTile(world_x - N3D_PLAYER_COLLISION_HALF_EXTENT);
    const int tile_x_b =
        N3D_RE_ProbeTile(world_x + N3D_PLAYER_COLLISION_HALF_EXTENT);

    return N3D_RE_TestProbePair(
        tile_x_a, tile_y,
        tile_x_b, tile_y,
        signed_step,
        callbacks,
        resolved);
}

n3d_player_move_result N3D_RE_MovePlayerWorldDelta(
    int delta_x,
    int delta_y,
    const n3d_collision_callbacks* callbacks)
{
    n3d_player_move_result result = {0};
    result.requested_x = delta_x;
    result.requested_y = delta_y;

    int32_t x = n3d_player.world_x;
    int32_t y = n3d_player.world_y;

    const int abs_x = abs(delta_x);
    const int abs_y = abs(delta_y);
    const int step_x = delta_x < 0 ? -1 : delta_x > 0 ? 1 : 0;
    const int step_y = delta_y < 0 ? -1 : delta_y > 0 ? 1 : 0;

    /*
     * Advance the dominant axis every iteration and the minor axis according
     * to an integer phase accumulator. Axis collision is independent: a blocked
     * component does not cancel the other component, preserving wall sliding.
     */
    if(abs_x >= abs_y)
    {
        int error = 0;

        for(int i = 0; i < abs_x; ++i)
        {
            if(step_x)
            {
                int resolved = 0;
                ++result.x_attempts;
                const int accepted =
                    N3D_RE_TestPlayerXSubstep(
                        x, y, step_x, callbacks, &resolved);

                if(!resolved)
                    result.unresolved_collision = 1;

                if(accepted)
                {
                    x += accepted;
                    result.accepted_x += accepted;
                }
                else
                {
                    ++result.x_blocked;
                }
            }

            error += abs_y;
            if(step_y && abs_x > 0 && error >= abs_x)
            {
                error -= abs_x;

                int resolved = 0;
                ++result.y_attempts;
                const int accepted =
                    N3D_RE_TestPlayerYSubstep(
                        x, y, step_y, callbacks, &resolved);

                if(!resolved)
                    result.unresolved_collision = 1;

                if(accepted)
                {
                    y += accepted;
                    result.accepted_y += accepted;
                }
                else
                {
                    ++result.y_blocked;
                }
            }
        }
    }
    else
    {
        int error = 0;

        for(int i = 0; i < abs_y; ++i)
        {
            if(step_y)
            {
                int resolved = 0;
                ++result.y_attempts;
                const int accepted =
                    N3D_RE_TestPlayerYSubstep(
                        x, y, step_y, callbacks, &resolved);

                if(!resolved)
                    result.unresolved_collision = 1;

                if(accepted)
                {
                    y += accepted;
                    result.accepted_y += accepted;
                }
                else
                {
                    ++result.y_blocked;
                }
            }

            error += abs_x;
            if(step_x && abs_y > 0 && error >= abs_y)
            {
                error -= abs_y;

                int resolved = 0;
                ++result.x_attempts;
                const int accepted =
                    N3D_RE_TestPlayerXSubstep(
                        x, y, step_x, callbacks, &resolved);

                if(!resolved)
                    result.unresolved_collision = 1;

                if(accepted)
                {
                    x += accepted;
                    result.accepted_x += accepted;
                }
                else
                {
                    ++result.x_blocked;
                }
            }
        }
    }

    uint8_t event_id = 0;
    if(N3D_RE_CommitPlayerWorldPosition(x, y, &event_id))
        result.entered_tile_event = event_id;
    else
        result.unresolved_collision = 1;

    return result;
}


static int N3D_RE_Abs16(int16_t value)
{
    return value < 0 ? -(int)value : (int)value;
}

int N3D_RE_InitLineStateFromAngle(
    int angle_degrees,
    n3d_line_state* state)
{
    if(!state || !n3d_trig.loaded)
        return 0;

    const int angle = N3D_RE_NormalizeAngle(angle_degrees);
    const int16_t sin_q10 = N3D_RE_SinQ10(angle);
    const int16_t cos_q10 = N3D_RE_CosQ10(angle);

    const int abs_sin = N3D_RE_Abs16(sin_q10);
    const int abs_cos = N3D_RE_Abs16(cos_q10);

    state->sin_q10 = sin_q10;
    state->cos_q10 = cos_q10;

    /*
     * Exact E516 setup:
     *   axis=1 when |cos| < |sin|, otherwise axis=0.
     *   error = 2*minor - major
     *   4C0A = 2*minor
     *   4C0C = 2*(minor-major)
     */
    if(abs_cos < abs_sin)
    {
        state->axis_flag = 1;
        state->error = (int16_t)(2 * abs_cos - abs_sin);
        state->twice_minor = (int16_t)(2 * abs_cos);
        state->twice_minor_minus_major =
            (int16_t)(2 * (abs_cos - abs_sin));
    }
    else
    {
        state->axis_flag = 0;
        state->error = (int16_t)(2 * abs_sin - abs_cos);
        state->twice_minor = (int16_t)(2 * abs_sin);
        state->twice_minor_minus_major =
            (int16_t)(2 * (abs_sin - abs_cos));
    }

    /*
     * Exact projectile spawn signs, shared with the same E516 line state:
     *   +08 -> X step = sign(sin): + for angle<180, else -
     *   +0A -> Y step = -sign(cos):
     *          + for 90<angle<270, else -
     * Zero components carry an unused sign, exactly as the original does.
     */
    state->step_x = angle < 180 ? 1 : -1;
    state->step_y = (angle > 90 && angle < 270) ? 1 : -1;

    return 1;
}

static void N3D_RE_RecordAxisAttempt(
    n3d_player_move_result* result,
    int is_x,
    int signed_step,
    int accepted,
    int resolved)
{
    if(!result)
        return;

    if(!resolved)
        result->unresolved_collision = 1;

    if(is_x)
    {
        result->requested_x += signed_step;
        ++result->x_attempts;
        if(accepted)
            result->accepted_x += signed_step;
        else
            ++result->x_blocked;
    }
    else
    {
        result->requested_y += signed_step;
        ++result->y_attempts;
        if(accepted)
            result->accepted_y += signed_step;
        else
            ++result->y_blocked;
    }
}

n3d_player_move_result N3D_RE_MovePlayerAngleSubsteps(
    int angle_degrees,
    uint16_t major_substeps,
    const n3d_collision_callbacks* callbacks)
{
    n3d_player_move_result result = {0};
    n3d_line_state line = {0};

    if(major_substeps == 0 ||
       !N3D_RE_InitLineStateFromAngle(angle_degrees, &line))
    {
        result.unresolved_collision = n3d_trig.loaded ? 0 : 1;
        return result;
    }

    int32_t x = n3d_player.world_x;
    int32_t y = n3d_player.world_y;

    for(uint16_t i = 0; i < major_substeps; ++i)
    {
        /*
         * Exact 9D30 axis meaning:
         *   axis != 0 -> major X uses slot+08 (sign(sin))
         *   axis == 0 -> major Y uses slot+0A (-sign(cos))
         */
        if(line.axis_flag != 0)
        {
            int resolved = 0;
            const int accepted =
                N3D_RE_TestPlayerXSubstep(
                    x, y, line.step_x, callbacks, &resolved);

            N3D_RE_RecordAxisAttempt(
                &result, 1, line.step_x, accepted != 0, resolved);

            if(accepted)
                x += accepted;
        }
        else
        {
            int resolved = 0;
            const int accepted =
                N3D_RE_TestPlayerYSubstep(
                    x, y, line.step_y, callbacks, &resolved);

            N3D_RE_RecordAxisAttempt(
                &result, 0, line.step_y, accepted != 0, resolved);

            if(accepted)
                y += accepted;
        }

        /*
         * Exact 9D30 error update:
         *   error < 0: error += 2*minor, no secondary step
         *   otherwise: error += 2*(minor-major), then secondary axis step
         */
        if(line.error < 0)
        {
            line.error =
                (int16_t)(line.error + line.twice_minor);
        }
        else
        {
            line.error =
                (int16_t)(
                    line.error + line.twice_minor_minus_major);

            if(line.axis_flag != 0)
            {
                int resolved = 0;
                const int accepted =
                    N3D_RE_TestPlayerYSubstep(
                        x, y, line.step_y, callbacks, &resolved);

                N3D_RE_RecordAxisAttempt(
                    &result, 0, line.step_y, accepted != 0, resolved);

                if(accepted)
                    y += accepted;
            }
            else
            {
                int resolved = 0;
                const int accepted =
                    N3D_RE_TestPlayerXSubstep(
                        x, y, line.step_x, callbacks, &resolved);

                N3D_RE_RecordAxisAttempt(
                    &result, 1, line.step_x, accepted != 0, resolved);

                if(accepted)
                    x += accepted;
            }
        }
    }

    uint8_t event_id = 0;
    if(N3D_RE_CommitPlayerWorldPosition(x, y, &event_id))
        result.entered_tile_event = event_id;
    else
        result.unresolved_collision = 1;

    return result;
}
