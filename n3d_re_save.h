#ifndef N3D_RE_SAVE_H
#define N3D_RE_SAVE_H

#include "n3d_re_runtime.h"
#include "n3d_re_door.h"
#include "n3d_re_special_runtime.h"
#include "n3d_re_guard.h"

#include <stddef.h>
#include <stdint.h>

#define N3D_USER_SAVE_SLOT_BYTES 0xD6E7u

#define N3D_SAVE_OBJECT_OFFSET 0x8DF3u
#define N3D_SAVE_OBJECT_BYTES  0x2648u

#define N3D_SAVE_GUARD_OFFSET 0xB43Bu
#define N3D_SAVE_GUARD_BYTES  0x0A28u

#define N3D_SAVE_DOOR_OFFSET 0xBE63u
#define N3D_SAVE_DOOR_BYTES  0x0580u

#define N3D_SAVE_PANEL_ACTIVATION_OFFSET 0xC3E3u
#define N3D_SAVE_PANEL_ACTIVATION_BYTES  0x0020u

#define N3D_SAVE_PROJECTILE_OFFSET 0xC403u
#define N3D_SAVE_PROJECTILE_BYTES  0x0150u

#define N3D_SAVE_GLOBAL_51A4_OFFSET 0xC553u
#define N3D_SAVE_GLOBAL_51A4_BYTES  0x0008u

#define N3D_SAVE_PUSH_OFFSET 0xC55Bu
#define N3D_SAVE_PUSH_BYTES  0x0048u

#define N3D_SAVE_GUARD_WAKE_OFFSET 0xD5A3u
#define N3D_SAVE_GUARD_WAKE_BYTES  0x0040u

_Static_assert(N3D_MAX_OBJECTS * N3D_OBJECT_STRIDE == N3D_SAVE_OBJECT_BYTES,
               "OBJECT save block size mismatch");
_Static_assert(N3D_MAX_GUARDS * N3D_GUARD_STRIDE == N3D_SAVE_GUARD_BYTES,
               "GUARD save block size mismatch");
_Static_assert(N3D_MAX_DOORS * N3D_DOOR_RECORD_SIZE == N3D_SAVE_DOOR_BYTES,
               "DOOR save block size mismatch");
_Static_assert(N3D_MAX_PANELS == N3D_SAVE_PANEL_ACTIVATION_BYTES,
               "PANEL activation save block size mismatch");
_Static_assert(N3D_MAX_PROJECTILES * N3D_PROJECTILE_STRIDE == N3D_SAVE_PROJECTILE_BYTES,
               "projectile save block size mismatch");
_Static_assert(N3D_MAX_PUSHES * N3D_PUSH_RECORD_SIZE == N3D_SAVE_PUSH_BYTES,
               "PUSH save block size mismatch");
_Static_assert(N3D_GUARD_WAKE_CACHE_SIZE == N3D_SAVE_GUARD_WAKE_BYTES,
               "guard wake save block size mismatch");

typedef struct n3d_known_save_blocks
{
    n3d_object_record objects[N3D_MAX_OBJECTS];
    n3d_guard_record guards[N3D_MAX_GUARDS];
    n3d_door_record doors[N3D_MAX_DOORS];
    uint8_t panel_activation[N3D_MAX_PANELS];
    n3d_projectile_record projectiles[N3D_MAX_PROJECTILES];
    uint8_t global_51a4[N3D_SAVE_GLOBAL_51A4_BYTES];
    n3d_push_record pushes[N3D_MAX_PUSHES];
    uint8_t guard_wake_cache[N3D_GUARD_WAKE_CACHE_SIZE];
} n3d_known_save_blocks;

/*
 * Reads/writes only the verified blocks above. Bytes outside those ranges are
 * untouched. This codec does not infer runtime counts and does not rebuild
 * Win16 far pointers; those are separate load-time responsibilities.
 */
int N3D_RE_ReadKnownSaveBlocks(
    const uint8_t* slot,
    size_t slot_size,
    n3d_known_save_blocks* out_blocks);

int N3D_RE_WriteKnownSaveBlocks(
    uint8_t* slot,
    size_t slot_size,
    const n3d_known_save_blocks* blocks);

#endif
