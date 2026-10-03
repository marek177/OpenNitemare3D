#include "n3d_re_wall_explosion.h"

#include "n3d_re_definitions.h"

#include <string.h>

n3d_exploding_wall_record
    n3d_exploding_walls[N3D_EXPLODING_WALL_CELL_COUNT];

static size_t N3D_RE_ExplosionCellIndex(
    uint8_t x,
    uint8_t y)
{
    return (size_t)y * N3D_MAP_WIDTH + x;
}

static int N3D_RE_FirstWallIdForClass(
    uint8_t wall_class,
    uint8_t* raw_wall_id)
{
    if(!raw_wall_id)
        return 0;

    for(int id = 0; id < 256; ++id)
    {
        if(n3d_wall_mapping_known[id] &&
           n3d_wall_mapped_type[id] == wall_class)
        {
            *raw_wall_id = (uint8_t)id;
            return 1;
        }
    }

    return 0;
}

void N3D_RE_ResetExplodingWalls(void)
{
    memset(n3d_exploding_walls, 0, sizeof(n3d_exploding_walls));
}

const n3d_exploding_wall_record*
N3D_RE_ExplodingWallAt(uint8_t x, uint8_t y)
{
    if(x >= N3D_MAP_WIDTH || y >= N3D_MAP_HEIGHT)
        return NULL;

    const n3d_exploding_wall_record* wall =
        &n3d_exploding_walls[N3D_RE_ExplosionCellIndex(x, y)];

    return wall->active ? wall : NULL;
}

int N3D_RE_ExplodingWallVisual(
    uint8_t x,
    uint8_t y,
    uint8_t* sequence_wall_id,
    uint8_t* frame)
{
    const n3d_exploding_wall_record* wall =
        N3D_RE_ExplodingWallAt(x, y);
    if(!wall)
        return 0;

    if(sequence_wall_id)
        *sequence_wall_id = wall->sequence_wall_id;
    if(frame)
        *frame = wall->frame;
    return 1;
}

int N3D_RE_ExplodingWallRuntimeClass(
    uint8_t x,
    uint8_t y,
    uint8_t* wall_class)
{
    if(!N3D_RE_ExplodingWallAt(x, y))
        return 0;

    if(wall_class)
        *wall_class = N3D_EXPLODING_WALL_RUNTIME_CLASS;
    return 1;
}

int N3D_RE_StartExplodingWall(
    uint8_t x,
    uint8_t y,
    uint32_t now_ms)
{
    if(x >= N3D_MAP_WIDTH || y >= N3D_MAP_HEIGHT ||
       !n3d_img.loaded)
        return 0;

    n3d_exploding_wall_record* existing =
        &n3d_exploding_walls[N3D_RE_ExplosionCellIndex(x, y)];
    if(existing->active)
        return 1;

    const n3d_map_cell* cell = N3D_RE_MapCell(x, y);
    if(!cell ||
       !N3D_RE_WallMappingKnown(cell->wall))
        return 0;

    const uint8_t source_class =
        n3d_wall_mapped_type[cell->wall];

    uint8_t sequence_wall_id = cell->wall;
    uint8_t first_frame = 0;

    if(source_class == N3D_EXPLODABLE_WALL1_CLASS)
    {
        if(!N3D_RE_FirstWallIdForClass(
                N3D_EXPLODING_WALL_RUNTIME_CLASS,
                &sequence_wall_id))
            return 0;

        first_frame = 0;
    }
    else if(source_class == N3D_EXPLODABLE_WALL2_CLASS)
    {
        /*
         * 9B64 selects the WALL_EX2 wall sequence already present in the map
         * and begins at frame one.
         */
        first_frame = 1;
    }
    else
    {
        return 0;
    }

    const n3d_img_sequence_def* sequence =
        N3D_RE_WallSequence(&n3d_img, sequence_wall_id);
    if(!sequence ||
       sequence->frame_count == 0 ||
       first_frame >= sequence->frame_count)
        return 0;

    memset(existing, 0, sizeof(*existing));
    existing->active = 1;
    existing->source_wall_id = cell->wall;
    existing->source_wall_class = source_class;
    existing->sequence_wall_id = sequence_wall_id;
    existing->frame = first_frame;
    existing->animation_deadline_ms =
        now_ms + (uint32_t)sequence->interval_ms;

    return 1;
}

unsigned N3D_RE_UpdateExplodingWalls(uint32_t now_ms)
{
    if(!n3d_img.loaded)
        return 0;

    unsigned completed = 0;

    for(size_t cell_index = 0;
        cell_index < N3D_EXPLODING_WALL_CELL_COUNT;
        ++cell_index)
    {
        n3d_exploding_wall_record* wall =
            &n3d_exploding_walls[cell_index];

        if(!wall->active ||
           wall->animation_deadline_ms > now_ms)
            continue;

        const n3d_img_sequence_def* sequence =
            N3D_RE_WallSequence(
                &n3d_img,
                wall->sequence_wall_id);

        if(!sequence || sequence->frame_count == 0)
        {
            wall->active = 0;
            continue;
        }

        const unsigned next =
            (unsigned)wall->frame + 1u;

        if(next >= sequence->frame_count)
        {
            /*
             * Win16 FUN_1018_3C0C clears wall bytes along the completed VEC.
             * The current C-rewrite world is still a tile bridge, so clear the
             * directly impacted MAP cell only. Merged-VEC span cleanup remains
             * a renderer-integration TODO rather than being guessed here.
             */
            n3d_map[cell_index].wall = 0;
            wall->active = 0;
            ++completed;
            continue;
        }

        wall->frame = (uint8_t)next;
        wall->animation_deadline_ms =
            now_ms + (uint32_t)sequence->interval_ms;
    }

    return completed;
}
