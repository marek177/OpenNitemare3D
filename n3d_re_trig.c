#include "n3d_re_trig.h"

#include <stdio.h>
#include <string.h>

n3d_trig_q10 n3d_trig;

static int16_t N3D_RE_ReadS16LE(const uint8_t* bytes)
{
    return (int16_t)((uint16_t)bytes[0] |
                     ((uint16_t)bytes[1] << 8));
}

int N3D_RE_NormalizeAngle(int degrees)
{
    int value = degrees % N3D_TRIG_ANGLE_COUNT;
    if(value < 0)
        value += N3D_TRIG_ANGLE_COUNT;
    return value;
}

void N3D_RE_ClearTrigQ10(void)
{
    memset(&n3d_trig, 0, sizeof(n3d_trig));
}

int N3D_RE_LoadTrigQ10(const char* path)
{
    if(!path)
        return 0;

    FILE* file = fopen(path, "rb");
    if(!file)
        return 0;

    uint8_t bytes[N3D_TRIG_FILE_BYTES];
    const size_t read_count =
        fread(bytes, 1, sizeof(bytes), file);

    const int extra = fgetc(file);
    fclose(file);

    if(read_count != sizeof(bytes) || extra != EOF)
        return 0;

    N3D_RE_ClearTrigQ10();

    for(int i = 0; i < N3D_TRIG_ANGLE_COUNT; ++i)
    {
        n3d_trig.sin_q10[i] =
            N3D_RE_ReadS16LE(&bytes[i * 2]);

        n3d_trig.cos_q10[i] =
            N3D_RE_ReadS16LE(
                &bytes[N3D_TRIG_ANGLE_COUNT * 2 + i * 2]);
    }

    /*
     * Exact Win16 1.10 sanity anchors. Refuse a malformed/wrong table instead
     * of silently falling back to host floating-point trigonometry.
     */
    if(n3d_trig.sin_q10[0] != 0 ||
       n3d_trig.sin_q10[45] != 724 ||
       n3d_trig.sin_q10[90] != 1024 ||
       n3d_trig.cos_q10[0] != 1024 ||
       n3d_trig.cos_q10[45] != 724 ||
       n3d_trig.cos_q10[90] != 0)
    {
        N3D_RE_ClearTrigQ10();
        return 0;
    }

    n3d_trig.loaded = 1;
    return 1;
}

int16_t N3D_RE_SinQ10(int degrees)
{
    if(!n3d_trig.loaded)
        return 0;

    return n3d_trig.sin_q10[N3D_RE_NormalizeAngle(degrees)];
}

int16_t N3D_RE_CosQ10(int degrees)
{
    if(!n3d_trig.loaded)
        return 0;

    return n3d_trig.cos_q10[N3D_RE_NormalizeAngle(degrees)];
}

uint8_t N3D_RE_CoarseOctant(int degrees)
{
    return (uint8_t)(N3D_RE_NormalizeAngle(degrees) / 45);
}

uint8_t N3D_RE_RoundedOctant(int degrees)
{
    const int angle = N3D_RE_NormalizeAngle(degrees);
    return (uint8_t)(((2 * angle / 45 + 1) >> 1) & 7);
}
