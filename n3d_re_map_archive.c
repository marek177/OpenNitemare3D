#include "n3d_re_map_archive.h"

#include "n3d_re_collision.h"
#include "n3d_re_definitions.h"

#include <stdio.h>
#include <string.h>

n3d_map_archive_header n3d_map_archive;

static uint16_t N3D_RE_MapReadU16(const uint8_t* p)
{
    return (uint16_t)(
        (uint16_t)p[0] |
        ((uint16_t)p[1] << 8));
}

void N3D_RE_ResetMapArchiveHeader(void)
{
    memset(&n3d_map_archive, 0, sizeof(n3d_map_archive));
}

int N3D_RE_LoadMapArchiveHeader(
    const char* path,
    n3d_map_archive_header* out_header)
{
    if(!path || !out_header)
        return 0;

    FILE* file = fopen(path, "rb");
    if(!file)
        return 0;

    uint8_t bytes[N3D_MAP_HEADER_BYTES];
    const size_t got =
        fread(bytes, 1, sizeof(bytes), file);
    fclose(file);

    if(got != sizeof(bytes))
        return 0;

    n3d_map_archive_header result;
    memset(&result, 0, sizeof(result));

    result.level_count =
        N3D_RE_MapReadU16(bytes + 0x0000);
    memcpy(result.wall_class, bytes + 0x0002, 256);
    memcpy(result.object_class, bytes + 0x0102, 256);
    result.loaded = 1;

    *out_header = result;
    return 1;
}

void N3D_RE_ApplyMapArchiveClassTables(
    const n3d_map_archive_header* header)
{
    if(!header || !header->loaded)
        return;

    for(int raw = 0; raw < 256; ++raw)
    {
        const uint8_t wall_class =
            header->wall_class[raw];
        const uint8_t object_class =
            header->object_class[raw];

        n3d_wall_mapped_type[raw] = wall_class;
        n3d_wall_mapping_known[raw] = 1;
        n3d_wall_property_known[raw] = 1;
        n3d_wall_property_resolved[raw] =
            N3D_RE_WallPropertiesForMappedType(wall_class);

        n3d_object_mapped_type[raw] = object_class;
        n3d_object_mapping_known[raw] = 1;
        n3d_object_property_known[raw] = 1;
        n3d_object_property_resolved[raw] =
            N3D_RE_ObjectPropertiesForMappedType(object_class);
    }
}

int N3D_RE_LoadMapEpisodeHeader(uint8_t episode)
{
    char path[32];
    snprintf(path, sizeof(path), "MAP.%u", episode);

    n3d_map_archive_header loaded;
    if(!N3D_RE_LoadMapArchiveHeader(path, &loaded))
        return 0;

    n3d_map_archive = loaded;
    N3D_RE_ApplyMapArchiveClassTables(&n3d_map_archive);
    return 1;
}

int N3D_RE_FindFirstWallIdForClass(
    const n3d_map_archive_header* header,
    uint8_t wall_class,
    uint8_t* raw_wall_id)
{
    if(!header || !header->loaded || !raw_wall_id)
        return 0;

    for(int id = 0; id < 256; ++id)
    {
        if(header->wall_class[id] == wall_class)
        {
            *raw_wall_id = (uint8_t)id;
            return 1;
        }
    }

    return 0;
}

int N3D_RE_FindFirstObjectIdForClass(
    const n3d_map_archive_header* header,
    uint8_t object_class,
    uint8_t* raw_object_id)
{
    if(!header || !header->loaded || !raw_object_id)
        return 0;

    for(int id = 0; id < 256; ++id)
    {
        if(header->object_class[id] == object_class)
        {
            *raw_object_id = (uint8_t)id;
            return 1;
        }
    }

    return 0;
}

int N3D_RE_MapWallClassVariant(
    const n3d_map_archive_header* header,
    uint8_t raw_wall_id,
    uint8_t expected_wall_class,
    uint8_t* variant)
{
    if(!header || !header->loaded || !variant ||
       header->wall_class[raw_wall_id] != expected_wall_class)
        return 0;

    for(int id = 0; id <= raw_wall_id; ++id)
    {
        if(header->wall_class[id] == expected_wall_class)
        {
            *variant =
                (uint8_t)(raw_wall_id - id);
            return 1;
        }
    }

    return 0;
}

int N3D_RE_MapObjectClassVariant(
    const n3d_map_archive_header* header,
    uint8_t raw_object_id,
    uint8_t* object_class,
    uint8_t* variant)
{
    if(!header || !header->loaded ||
       !object_class || !variant)
        return 0;

    const uint8_t cls =
        header->object_class[raw_object_id];

    for(int id = 0; id <= raw_object_id; ++id)
    {
        if(header->object_class[id] == cls)
        {
            *object_class = cls;
            *variant =
                (uint8_t)(raw_object_id - id);
            return 1;
        }
    }

    return 0;
}
