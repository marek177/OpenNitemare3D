#ifndef N3D_RE_TIMING_H
#define N3D_RE_TIMING_H

#include <stdint.h>

#define N3D_TIMING_CALIBRATION_SAMPLES 5

typedef struct n3d_timing_parameters
{
    uint16_t raw_mean_ms;
    uint16_t effective_ms;
    uint16_t slow_updates_per_second;
    uint16_t movement_substeps;
    uint16_t turn_degrees;
    uint16_t projectile_substeps;
} n3d_timing_parameters;

typedef struct n3d_timing_calibration
{
    uint32_t total_ms;
    uint8_t sample_count;
    uint8_t complete;
} n3d_timing_calibration;

extern n3d_timing_parameters n3d_timing;
extern n3d_timing_calibration n3d_timing_calibration_state;

n3d_timing_parameters N3D_RE_ComputeTimingParameters(uint16_t raw_mean_ms);
void N3D_RE_SetTimingFromRawMean(uint16_t raw_mean_ms);
void N3D_RE_ResetTimingCalibration(void);

/*
 * Returns 1 exactly once, when the fifth non-zero frame interval completes
 * the original elapsed/5 calibration and updates n3d_timing.
 */
int N3D_RE_SampleTimingFrame(uint16_t frame_ms);

#endif
