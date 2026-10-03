#include "n3d_re_controls.h"

#include "n3d_re_trig.h"

n3d_control_steps N3D_RE_ControlStepsForInput(uint16_t input_mask)
{
    n3d_control_steps result = {
        n3d_timing.movement_substeps,
        n3d_timing.turn_degrees
    };

    /*
     * Exact FUN_1010_9806 ordering:
     * 0x20 doubles both base values, then 0x40 forces both to one.
     */
    if(input_mask & N3D_INPUT_FAST)
    {
        result.movement_substeps =
            (uint16_t)(result.movement_substeps * 2u);
        result.turn_degrees =
            (uint16_t)(result.turn_degrees * 2u);
    }

    if(input_mask & N3D_INPUT_FINE)
    {
        result.movement_substeps = 1;
        result.turn_degrees = 1;
    }

    return result;
}

int N3D_RE_ForwardAngle(int player_angle)
{
    return N3D_RE_NormalizeAngle(player_angle);
}

int N3D_RE_BackwardAngle(int player_angle)
{
    return N3D_RE_NormalizeAngle(player_angle + 180);
}

int N3D_RE_StrafeLeftAngle(int player_angle)
{
    return N3D_RE_NormalizeAngle(player_angle + 270);
}

int N3D_RE_StrafeRightAngle(int player_angle)
{
    return N3D_RE_NormalizeAngle(player_angle + 90);
}

int N3D_RE_LeftTurnDelta(uint16_t input_mask)
{
    if(input_mask & N3D_INPUT_STRAFE)
        return 0;

    const n3d_control_steps steps =
        N3D_RE_ControlStepsForInput(input_mask);
    return -(int)steps.turn_degrees;
}

int N3D_RE_RightTurnDelta(uint16_t input_mask)
{
    if(input_mask & N3D_INPUT_STRAFE)
        return 0;

    const n3d_control_steps steps =
        N3D_RE_ControlStepsForInput(input_mask);
    return (int)steps.turn_degrees;
}
