#ifndef N3D_RE_TIMING_H
#define N3D_RE_TIMING_H

#include <stdint.h>

typedef struct n3d_timing_parameters
{
    uint16_t raw_mean_ms;
    uint16_t effective_ms;
    uint16_t slow_updates_per_second;
    uint16_t movement_substeps;
    uint16_t turn_degrees;
    uint16_t projectile_substeps;
} n3d_timing_parameters;

extern n3d_timing_parameters n3d_timing;

n3d_timing_parameters N3D_RE_ComputeTimingParameters(uint16_t raw_mean_ms);
void N3D_RE_SetTimingFromRawMean(uint16_t raw_mean_ms);

#endif
