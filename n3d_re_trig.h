#ifndef N3D_RE_TRIG_H
#define N3D_RE_TRIG_H

#include <stdint.h>

#define N3D_TRIG_ANGLE_COUNT 360
#define N3D_TRIG_Q10_SCALE 1024
#define N3D_TRIG_FILE_BYTES (N3D_TRIG_ANGLE_COUNT * 2 * 2)

typedef struct n3d_trig_q10
{
    int16_t sin_q10[N3D_TRIG_ANGLE_COUNT];
    int16_t cos_q10[N3D_TRIG_ANGLE_COUNT];
    uint8_t loaded;
} n3d_trig_q10;

extern n3d_trig_q10 n3d_trig;

int N3D_RE_NormalizeAngle(int degrees);
int N3D_RE_LoadTrigQ10(const char* path);
void N3D_RE_ClearTrigQ10(void);
int16_t N3D_RE_SinQ10(int degrees);
int16_t N3D_RE_CosQ10(int degrees);
uint8_t N3D_RE_CoarseOctant(int degrees);
uint8_t N3D_RE_RoundedOctant(int degrees);

#endif
