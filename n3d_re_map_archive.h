#ifndef N3D_RE_MAP_ARCHIVE_H
#define N3D_RE_MAP_ARCHIVE_H

#include "n3d_re_runtime.h"

#include <stdint.h>

typedef struct n3d_map_archive_header
{
    uint16_t level_count;
    uint8_t wall_class[256];
    uint8_t object_class[256];
    uint8_t loaded;
} n3d_map_archive_header;

extern n3d_map_archive_header n3d_map_archive;

/*
 * Exact 514-byte MAP archive header:
 *   +0000 uint16 level count
 *   +0002 wallClass[256]
 *   +0102 objectClass[256]
 */
void N3D_RE_ResetMapArchiveHeader(void);
int N3D_RE_LoadMapArchiveHeader(
    const char* path,
    n3d_map_archive_header* out_header);
int N3D_RE_LoadMapEpisodeHeader(uint8_t episode);

/*
 * Installs the MAP header's exact raw-ID -> runtime-class tables as the
 * authoritative mapping/property source used by reconstructed gameplay.
 */
void N3D_RE_ApplyMapArchiveClassTables(
    const n3d_map_archive_header* header);

int N3D_RE_FindFirstWallIdForClass(
    const n3d_map_archive_header* header,
    uint8_t wall_class,
    uint8_t* raw_wall_id);

int N3D_RE_FindFirstObjectIdForClass(
    const n3d_map_archive_header* header,
    uint8_t object_class,
    uint8_t* raw_object_id);

int N3D_RE_MapWallClassVariant(
    const n3d_map_archive_header* header,
    uint8_t raw_wall_id,
    uint8_t expected_wall_class,
    uint8_t* variant);

int N3D_RE_MapObjectClassVariant(
    const n3d_map_archive_header* header,
    uint8_t raw_object_id,
    uint8_t* object_class,
    uint8_t* variant);

#endif
