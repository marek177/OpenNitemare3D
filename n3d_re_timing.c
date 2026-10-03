#include "n3d_re_timing.h"

n3d_timing_parameters n3d_timing;

static uint16_t max_u16(uint16_t a, uint16_t b)
{
    return a > b ? a : b;
}

n3d_timing_parameters N3D_RE_ComputeTimingParameters(uint16_t raw_mean_ms)
{
    n3d_timing_parameters result = {0};

    result.raw_mean_ms = raw_mean_ms;
    result.effective_ms = max_u16(raw_mean_ms, 40);

    const uint32_t d = result.effective_ms;

    result.slow_updates_per_second =
        (uint16_t)((1000u + (d / 2u)) / d);

    result.movement_substeps =
        (uint16_t)((d + 2u) / 4u);
    if(result.movement_substeps < 1)
        result.movement_substeps = 1;

    result.turn_degrees =
        (uint16_t)((360u * d + 1400u) / 2800u);
    if(result.turn_degrees < 1)
        result.turn_degrees = 1;

    result.projectile_substeps =
        (uint16_t)(2u * result.movement_substeps);

    return result;
}

void N3D_RE_SetTimingFromRawMean(uint16_t raw_mean_ms)
{
    n3d_timing = N3D_RE_ComputeTimingParameters(raw_mean_ms);
}
