#ifndef N3D_RE_GUARD_SOUNDS_H
#define N3D_RE_GUARD_SOUNDS_H

#include <stdint.h>

/* Exact NITE3W V1.10 SND.DAT selectors: B862/B5E4/B6A0. */
int N3D_RE_GuardAlertUsesRandom(uint8_t object_class);
int N3D_RE_GuardAttackUsesRandom(uint8_t object_class);
int N3D_RE_GuardDeathUsesRandom(uint8_t object_class);

int N3D_RE_GuardAlertSoundId(
    uint8_t object_class,
    uint16_t random_value);

int N3D_RE_GuardAttackSoundId(
    uint8_t object_class,
    uint16_t random_value);

int N3D_RE_GuardDeathSoundId(
    uint8_t object_class,
    uint16_t random_value);

#endif
