#include "n3d_re_guard_sounds.h"

int N3D_RE_GuardAlertUsesRandom(uint8_t c)
{
    return c == 0x09 ||
           c == 0x0A ||
           c == 0x0F ||
           c == 0x10;
}

int N3D_RE_GuardAttackUsesRandom(uint8_t c)
{
    return c == 0x0F ||
           c == 0x10 ||
           c == 0x16 ||
           (c >= 0x1B && c <= 0x1F);
}

int N3D_RE_GuardDeathUsesRandom(uint8_t c)
{
    return c == 0x0F || c == 0x10;
}

int N3D_RE_GuardAlertSoundId(
    uint8_t c,
    uint16_t r)
{
    switch(c)
    {
        case 0x08: return 0x22;
        case 0x09:
        case 0x0A: return (int)(r % 3u) + 0x38;
        case 0x0B: return 0x3B;
        case 0x0C: return 0x14;
        case 0x0D: return 0x15;
        case 0x0E: return 0x10;
        case 0x0F:
        case 0x10: return (int)(r % 2u) + 0x36;
        case 0x11: return 0x06;
        case 0x12: return 0x3F;
        case 0x13: return 0x3C;
        case 0x16: return 0x12;
        case 0x17: return 0x48;
        case 0x18: return 0x47;
        case 0x1A: return 0x49;
        case 0x1B:
        case 0x1C: return 0x0D;
        case 0x1D: return 0x38;
        case 0x1E:
        case 0x1F: return 0x3D;
        default: return 0;
    }
}

int N3D_RE_GuardAttackSoundId(
    uint8_t c,
    uint16_t r)
{
    switch(c)
    {
        case 0x09:
        case 0x0A: return 0x41;
        case 0x0B:
        case 0x1A: return 0x4E;
        case 0x0C:
        case 0x0D:
        case 0x0E:
        case 0x18: return 0x20;
        case 0x0F:
        case 0x10:
        case 0x16: return (int)(r % 3u) + 0x17;
        case 0x12: return 0x3E;
        case 0x13: return 0x40;
        case 0x17: return 0x1F;
        case 0x19: return 0x1D;
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F: return (int)(r % 4u) + 0x4B;
        default: return 0;
    }
}

int N3D_RE_GuardDeathSoundId(
    uint8_t c,
    uint16_t r)
{
    switch(c)
    {
        case 0x08:
        case 0x14: return 0x23;
        case 0x09: return 0x08;
        case 0x0A:
        case 0x12:
        case 0x13: return 0x07;
        case 0x0B: return 0x24;
        case 0x0C: return 0x04;
        case 0x0D: return 0x13;
        case 0x0E: return 0x0E;
        case 0x0F:
        case 0x10: return (int)(r % 3u) + 0x0B;
        case 0x17:
        case 0x18:
        case 0x1E:
        case 0x1F: return 0x46;
        case 0x1A: return 0x4A;
        case 0x1B:
        case 0x1C: return 0x02;
        case 0x1D: return 0x09;
        default: return 0;
    }
}
