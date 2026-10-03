#ifndef N3D_RE_WALL_EXPLOSION_H
#define N3D_RE_WALL_EXPLOSION_H

#include "n3d_re_img.h"
#include "n3d_re_runtime.h"

#include <stdint.h>

#define N3D_EXPLODING_WALL_CELL_COUNT (N3D_MAP_WIDTH * N3D_MAP_HEIGHT)
#define N3D_EXPLODING_WALL_RUNTIME_CLASS 0x2D
#define N3D_EXPLODABLE_WALL1_CLASS 0x2E
#define N3D_EXPLODABLE_WALL2_CLASS 0x2F

/*
 * Modern per-cell bridge for the recovered wall-explosion lifecycle.
 * The original animation is carried by wall/VEC runtime state; keeping this
 * overlay separate avoids inventing fields inside the still-partial 22-byte
 * door/panel records.
 */
typedef struct n3d_exploding_wall_record
{
    uint8_t active;
    uint8_t source_wall_id;
    uint8_t source_wall_class;
    uint8_t sequence_wall_id;
    uint8_t frame;
    uint32_t animation_deadline_ms;
} n3d_exploding_wall_record;

extern n3d_exploding_wall_record
    n3d_exploding_walls[N3D_EXPLODING_WALL_CELL_COUNT];

void N3D_RE_ResetExplodingWalls(void);

const n3d_exploding_wall_record*
N3D_RE_ExplodingWallAt(uint8_t x, uint8_t y);

int N3D_RE_ExplodingWallVisual(
    uint8_t x,
    uint8_t y,
    uint8_t* sequence_wall_id,
    uint8_t* frame);

int N3D_RE_ExplodingWallRuntimeClass(
    uint8_t x,
    uint8_t y,
    uint8_t* wall_class);

int N3D_RE_StartExplodingWall(
    uint8_t x,
    uint8_t y,
    uint32_t now_ms);

/*
 * Advances at most one SEQDEF frame per call, matching the update-loop
 * semantics used by the recovered runtime. Returns the number of cells whose
 * sequence completed during this call.
 */
unsigned N3D_RE_UpdateExplodingWalls(uint32_t now_ms);

#endif
