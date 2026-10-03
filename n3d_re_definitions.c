#include "n3d_re_definitions.h"
#include "n3d_re_collision.h"
#include "n3d_re_runtime.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

n3d_definition_table n3d_wall_definitions;
n3d_definition_table n3d_object_definitions;
uint8_t n3d_wall_mapped_type[N3D_DEFINITION_COUNT];
uint8_t n3d_object_mapped_type[N3D_DEFINITION_COUNT];
uint8_t n3d_wall_mapping_known[N3D_DEFINITION_COUNT];
uint8_t n3d_object_mapping_known[N3D_DEFINITION_COUNT];
uint8_t n3d_wall_property_known[N3D_DEFINITION_COUNT];
uint8_t n3d_object_property_known[N3D_DEFINITION_COUNT];
uint8_t n3d_wall_property_resolved[N3D_DEFINITION_COUNT];
uint8_t n3d_object_property_resolved[N3D_DEFINITION_COUNT];
uint8_t n3d_definition_episode;

void N3D_RE_ClearDefinitionTable(n3d_definition_table* table)
{
    if(!table)
        return;

    memset(table, 0, sizeof(*table));
}

static void N3D_RE_TrimLeft(char** text)
{
    if(!text || !*text)
        return;

    while(**text && isspace((unsigned char)**text))
        ++(*text);
}

static void N3D_RE_TrimRight(char* text)
{
    if(!text)
        return;

    size_t len = strlen(text);
    while(len > 0 && isspace((unsigned char)text[len - 1]))
        text[--len] = '\0';
}

int N3D_RE_ParseDefinitionLine(
    const char* line,
    n3d_definition_record* out_record)
{
    if(!line || !out_record)
        return 0;

    char copy[512];
    size_t len = strlen(line);
    if(len >= sizeof(copy))
        return 0;

    memcpy(copy, line, len + 1);

    char* cursor = copy;
    N3D_RE_TrimLeft(&cursor);
    N3D_RE_TrimRight(cursor);

    if(*cursor == '\0' || *cursor == '#' || *cursor == ';')
        return 0;

    char id_text[32] = {0};
    char visual[N3D_DEF_VISUAL_CODE_MAX] = {0};
    char image[N3D_DEF_IMAGE_NAME_MAX] = {0};
    char class_name[N3D_DEF_CLASS_NAME_MAX] = {0};
    int consumed = 0;

    const int fields = sscanf(
        cursor,
        "%31s %31s %63s %63s %n",
        id_text,
        visual,
        image,
        class_name,
        &consumed);

    if(fields < 4)
        return 0;

    char* end = NULL;
    unsigned long id = strtoul(id_text, &end, 16);
    if(!end || *end != '\0' || id > 0xFF)
        return 0;

    memset(out_record, 0, sizeof(*out_record));
    out_record->present = 1;
    out_record->id = (uint8_t)id;

    snprintf(out_record->visual_code, sizeof(out_record->visual_code), "%s", visual);
    snprintf(out_record->image_name, sizeof(out_record->image_name), "%s", image);
    snprintf(out_record->class_name, sizeof(out_record->class_name), "%s", class_name);

    char* description = cursor + consumed;
    N3D_RE_TrimLeft(&description);
    N3D_RE_TrimRight(description);
    snprintf(
        out_record->description,
        sizeof(out_record->description),
        "%s",
        description);

    return 1;
}

int N3D_RE_LoadDefinitionTable(
    const char* path,
    n3d_definition_table* table)
{
    if(!path || !table)
        return 0;

    FILE* file = fopen(path, "rb");
    if(!file)
        return 0;

    N3D_RE_ClearDefinitionTable(table);

    char line[512];
    while(fgets(line, sizeof(line), file))
    {
        n3d_definition_record record;
        if(!N3D_RE_ParseDefinitionLine(line, &record))
            continue;

        if(!table->record[record.id].present)
            ++table->count;

        table->record[record.id] = record;
    }

    fclose(file);
    return 1;
}

const n3d_definition_record* N3D_RE_FindDefinition(
    const n3d_definition_table* table,
    uint8_t id)
{
    if(!table || !table->record[id].present)
        return NULL;

    return &table->record[id];
}

int N3D_RE_DefinitionVariantIndex(
    const n3d_definition_table* table,
    uint8_t id,
    uint8_t* variant_index)
{
    if(!table || !variant_index)
        return 0;

    const n3d_definition_record* target =
        N3D_RE_FindDefinition(table, id);
    if(!target)
        return 0;

    uint8_t variant = 0;
    for(int raw = 0; raw < id; ++raw)
    {
        const n3d_definition_record* record =
            N3D_RE_FindDefinition(table, (uint8_t)raw);

        if(record &&
           strcmp(record->class_name, target->class_name) == 0)
        {
            ++variant;
        }
    }

    *variant_index = variant;
    return 1;
}

int N3D_RE_WallClassVariant(
    uint8_t raw_wall_id,
    uint8_t expected_mapped_type,
    uint8_t* variant_index)
{
    if(!variant_index ||
       !N3D_RE_WallMappingKnown(raw_wall_id) ||
       n3d_wall_mapped_type[raw_wall_id] != expected_mapped_type)
    {
        return 0;
    }

    for(int raw = 0; raw <= raw_wall_id; ++raw)
    {
        if(n3d_wall_mapping_known[raw] &&
           n3d_wall_mapped_type[raw] == expected_mapped_type)
        {
            *variant_index = (uint8_t)(raw_wall_id - raw);
            return 1;
        }
    }

    return 0;
}

int N3D_RE_AreaIdFromWallId(
    uint8_t raw_wall_id,
    uint8_t* area_id)
{
    return N3D_RE_WallClassVariant(
        raw_wall_id,
        0x44,
        area_id);
}

int N3D_RE_KnownWallMappedTypeForClass(
    const char* class_name,
    uint8_t* mapped_type)
{
    if(!class_name || !mapped_type)
        return 0;

    /*
     * Cross-bound by the 2026-09-28 class/map audit using the original
     * WALLS definitions on both DOS and Windows distributions.
     * 0x3D/0x3E remain intentionally unassigned until a named definition
     * or executable binding is recovered.
     */
    static const struct {
        const char* name;
        uint8_t mapped_type;
    } door_classes[] = {
        {"DOORV",   0x31},
        {"DOORH",   0x32},
        {"DOORVL",  0x33},
        {"DOORHL",  0x34},
        {"DOORVL2", 0x35},
        {"DOORHL2", 0x36},
        {"DOORVL3", 0x37},
        {"DOORHL3", 0x38},
        {"DOORVI",  0x39},
        {"DOORHI",  0x3A},
        {"DOORVR",  0x3B},
        {"DOORHR",  0x3C},
        {"DOORVC",  0x3F},
        {"DOORHC",  0x40}
    };

    for(size_t i = 0; i < sizeof(door_classes) / sizeof(door_classes[0]); ++i)
    {
        if(strcmp(class_name, door_classes[i].name) == 0)
        {
            *mapped_type = door_classes[i].mapped_type;
            return 1;
        }
    }

    /*
     * Canonical class names cross-bound by the 2026-09-28 WALLS audit.
     * Only names with an explicit class_hex association are listed.
     */
    static const struct {
        const char* name;
        uint8_t mapped_type;
    } wall_classes[] = {
        {"WALL",       0x01},
        {"REVWALL",    0x02},
        {"CONTROL",    0x03},
        {"ONE_SHOT",   0x07},
        {"SPECIAL1",   0x08},
        {"LEVEL_UP",   0x09},
        {"LEVEL_UP2",  0x0A},
        {"WARP_E1",    0x1D},
        {"WALL_EX",    0x2D},
        {"WALL_EX1",   0x2E},
        {"WALL_EX2",   0x2F},
        {"TURN",       0x41},
        {"RETREAT",    0x42},
        {"FLOOR",      0x44},
        {"SAFESPOT",   0x45},
        {"ACTIONSPOT", 0x46},
        {"TRIGGER1",   0x47},
        {"TRIGGER2",   0x48}
    };

    for(size_t i = 0; i < sizeof(wall_classes) / sizeof(wall_classes[0]); ++i)
    {
        if(strcmp(class_name, wall_classes[i].name) == 0)
        {
            *mapped_type = wall_classes[i].mapped_type;
            return 1;
        }
    }

    if(strncmp(class_name, "WARP_", 5) == 0 &&
       class_name[5] >= '1' && class_name[5] <= '8' &&
       class_name[6] == '\0')
    {
        *mapped_type = (uint8_t)(0x0D + (class_name[5] - '1'));
        return 1;
    }

    if(strcmp(class_name, "WARP_S1") == 0)
    {
        *mapped_type = 0x15;
        return 1;
    }

    if(strcmp(class_name, "WARP_S2") == 0)
    {
        *mapped_type = 0x16;
        return 1;
    }

    if(strncmp(class_name, "WARP_L", 6) == 0 &&
       class_name[6] >= '1' && class_name[6] <= '4' &&
       class_name[7] == '\0')
    {
        *mapped_type = (uint8_t)(0x19 + (class_name[6] - '1'));
        return 1;
    }

    return 0;
}

static int N3D_RE_IsKnownDynamicDoorClass(const char* class_name)
{
    static const char* classes[] = {
        "DOORV", "DOORH",
        "DOORVL", "DOORHL",
        "DOORVL2", "DOORHL2",
        "DOORVL3", "DOORHL3",
        "DOORVI", "DOORHI",
        "DOORVR", "DOORHR",
        "DOORVC", "DOORHC"
    };

    if(!class_name)
        return 0;

    for(size_t i = 0; i < sizeof(classes) / sizeof(classes[0]); ++i)
    {
        if(strcmp(class_name, classes[i]) == 0)
            return 1;
    }

    return 0;
}

int N3D_RE_KnownWallPropertyForClass(
    const char* class_name,
    uint8_t* property_flags)
{
    if(!class_name || !property_flags)
        return 0;

    if(N3D_RE_IsKnownDynamicDoorClass(class_name))
    {
        *property_flags = 0x0B; /* mapped type 0x31..0x40 */
        return 1;
    }

    uint8_t mapped_type = 0;
    if(N3D_RE_KnownWallMappedTypeForClass(class_name, &mapped_type))
    {
        *property_flags = N3D_RE_WallPropertiesForMappedType(mapped_type);
        return 1;
    }

    return 0;
}

int N3D_RE_KnownObjectMappedTypeForClass(
    const char* class_name,
    uint8_t* mapped_type)
{
    if(!class_name || !mapped_type)
        return 0;

    /*
     * Canonical OBJECTS class names from the 2026-09-28 class/map audit.
     * Unknown class_hex values 0x04/0x05/0x3E and unnamed entries remain open.
     */
    static const struct {
        const char* name;
        uint8_t mapped_type;
    } object_classes[] = {
        {"NULL",      0x00},
        {"START",     0x02},
        {"SECRET",    0x03},
        {"CAUSTIC",   0x07},
        {"SAFE",      0x26},
        {"TRUNK",     0x27},
        {"PUSH",      0x28},
        {"ACTION",    0x29},
        {"PERMEABLE", 0x2A},
        {"DUMB",      0x2B},
        {"ELEVATED",  0x2E},
        {"KEY",       0x2F},
        {"IDCARD",    0x30},
        {"FOOD",      0x33},
        {"WEAPON",    0x36},
        {"AMMO",      0x39},
        {"CRYSTALB",  0x3A},
        {"MAGICEYE",  0x3B},
        {"PENTAGRAM", 0x3C},
        {"SCROLL",    0x3D}
    };

    for(size_t i = 0; i < sizeof(object_classes) / sizeof(object_classes[0]); ++i)
    {
        if(strcmp(class_name, object_classes[i].name) == 0)
        {
            *mapped_type = object_classes[i].mapped_type;
            return 1;
        }
    }

    /*
     * Editor classes GUARD1..GUARD26 map directly to logical classes
     * 0x08..0x21. The numeric suffix is validated rather than assumed from
     * raw MAP IDs, so this also works across episode-specific raw IDs.
     */
    if(strncmp(class_name, "GUARD", 5) == 0)
    {
        char* endptr = NULL;
        const long guard_number = strtol(class_name + 5, &endptr, 10);

        if(endptr && *endptr == '\0' &&
           guard_number >= 1 && guard_number <= 26)
        {
            *mapped_type = (uint8_t)(0x07 + guard_number);
            return 1;
        }
    }

    return 0;
}

static void N3D_RE_SetKnownWallMapping(uint8_t raw_id, uint8_t mapped_type)
{
    n3d_wall_mapped_type[raw_id] = mapped_type;
    n3d_wall_mapping_known[raw_id] = 1;
    n3d_wall_property_known[raw_id] = 1;
    n3d_wall_property_resolved[raw_id] =
        N3D_RE_WallPropertiesForMappedType(mapped_type);
}

static void N3D_RE_SetKnownObjectMapping(uint8_t raw_id, uint8_t mapped_type)
{
    n3d_object_mapped_type[raw_id] = mapped_type;
    n3d_object_mapping_known[raw_id] = 1;
    n3d_object_property_known[raw_id] = 1;
    n3d_object_property_resolved[raw_id] =
        N3D_RE_ObjectPropertiesForMappedType(mapped_type);
}

void N3D_RE_RebuildKnownMappedTypes(void)
{
    memset(n3d_wall_mapped_type, N3D_MAPPED_TYPE_UNKNOWN,
           sizeof(n3d_wall_mapped_type));
    memset(n3d_object_mapped_type, N3D_MAPPED_TYPE_UNKNOWN,
           sizeof(n3d_object_mapped_type));
    memset(n3d_wall_mapping_known, 0, sizeof(n3d_wall_mapping_known));
    memset(n3d_object_mapping_known, 0, sizeof(n3d_object_mapping_known));
    memset(n3d_wall_property_known, 0, sizeof(n3d_wall_property_known));
    memset(n3d_object_property_known, 0, sizeof(n3d_object_property_known));
    memset(n3d_wall_property_resolved, 0, sizeof(n3d_wall_property_resolved));
    memset(n3d_object_property_resolved, 0, sizeof(n3d_object_property_resolved));

    /* Empty MAP bytes are fully known and have no collision properties. */
    N3D_RE_SetKnownWallMapping(0, 0);
    N3D_RE_SetKnownObjectMapping(0, 0);

    /*
     * Guard spawn ID -> runtime OBJECT class is independently recovered from
     * the executable and therefore can also seed the logical object mapping.
     */
    for(int id = 1; id < N3D_DEFINITION_COUNT; ++id)
    {
        uint8_t guard_class = 0;
        if(N3D_RE_GuardClassFromMapObject((uint8_t)id, &guard_class))
            N3D_RE_SetKnownObjectMapping((uint8_t)id, guard_class);
    }

    /*
     * Resolve only textual classes with direct EXE/data cross-binding.
     * Unknown classes remain explicit instead of receiving sequential guesses.
     */
    for(int id = 0; id < N3D_DEFINITION_COUNT; ++id)
    {
        const n3d_definition_record* wall =
            N3D_RE_FindDefinition(&n3d_wall_definitions, (uint8_t)id);
        if(wall)
        {
            uint8_t mapped = 0;
            if(N3D_RE_KnownWallMappedTypeForClass(
                    wall->class_name, &mapped))
            {
                N3D_RE_SetKnownWallMapping((uint8_t)id, mapped);
            }
            else
            {
                uint8_t property_flags = 0;
                if(N3D_RE_KnownWallPropertyForClass(
                        wall->class_name, &property_flags))
                {
                    n3d_wall_property_known[id] = 1;
                    n3d_wall_property_resolved[id] = property_flags;
                }
            }
        }

        const n3d_definition_record* object =
            N3D_RE_FindDefinition(&n3d_object_definitions, (uint8_t)id);
        if(object)
        {
            uint8_t mapped = 0;
            if(N3D_RE_KnownObjectMappedTypeForClass(
                    object->class_name, &mapped))
            {
                N3D_RE_SetKnownObjectMapping((uint8_t)id, mapped);
            }
        }
    }

    /*
     * Episode-1 raw IDs come from the upstream enum generated from WALLS.1;
     * their logical classes are independently verified in the EXE/MAP audit.
     * Do not apply these raw-ID bindings to other episodes.
     */
    if(n3d_definition_episode == 1)
    {
        N3D_RE_SetKnownWallMapping(184, 0x47); /* Trigger1 */
        N3D_RE_SetKnownWallMapping(185, 0x48); /* Trigger2 */
        N3D_RE_SetKnownWallMapping(254, 0x2E); /* explodable family 1 */
        N3D_RE_SetKnownWallMapping(255, 0x2F); /* explodable family 2 */
    }
}

int N3D_RE_WallMappingKnown(uint8_t raw_id)
{
    return n3d_wall_mapping_known[raw_id] != 0;
}

int N3D_RE_ObjectMappingKnown(uint8_t raw_id)
{
    return n3d_object_mapping_known[raw_id] != 0;
}

int N3D_RE_WallPropertyKnown(uint8_t raw_id)
{
    return n3d_wall_property_known[raw_id] != 0;
}

int N3D_RE_ObjectPropertyKnown(uint8_t raw_id)
{
    return n3d_object_property_known[raw_id] != 0;
}

static n3d_mapping_coverage N3D_RE_MappingCoverage(
    const n3d_definition_table* table,
    const uint8_t known[N3D_DEFINITION_COUNT])
{
    n3d_mapping_coverage coverage = {0, 0, 0};

    if(!table || !known)
        return coverage;

    for(int id = 0; id < N3D_DEFINITION_COUNT; ++id)
    {
        if(!table->record[id].present)
            continue;

        ++coverage.total;
        if(known[id])
            ++coverage.known;
        else
            ++coverage.unknown;
    }

    return coverage;
}

n3d_mapping_coverage N3D_RE_WallMappingCoverage(void)
{
    return N3D_RE_MappingCoverage(
        &n3d_wall_definitions,
        n3d_wall_mapping_known);
}

n3d_mapping_coverage N3D_RE_ObjectMappingCoverage(void)
{
    return N3D_RE_MappingCoverage(
        &n3d_object_definitions,
        n3d_object_mapping_known);
}

static void N3D_RE_DumpUnresolvedTable(
    const char* label,
    const n3d_definition_table* table,
    const uint8_t known[N3D_DEFINITION_COUNT])
{
    if(!label || !table || !known)
        return;

    for(int id = 0; id < N3D_DEFINITION_COUNT; ++id)
    {
        const n3d_definition_record* record = &table->record[id];
        if(!record->present || known[id])
            continue;

        printf(
            "UNRESOLVED %s raw=%02X class=%s image=%s desc=%s\n",
            label,
            id,
            record->class_name,
            record->image_name,
            record->description);
    }
}

void N3D_RE_DumpUnresolvedMappings(void)
{
    N3D_RE_DumpUnresolvedTable(
        "WALL",
        &n3d_wall_definitions,
        n3d_wall_mapping_known);

    N3D_RE_DumpUnresolvedTable(
        "OBJECT",
        &n3d_object_definitions,
        n3d_object_mapping_known);
}

int N3D_RE_LoadEpisodeDefinitions(uint8_t episode)
{
    n3d_definition_episode = episode;

    char walls_path[32];
    char objects_path[32];

    snprintf(walls_path, sizeof(walls_path), "WALLS.%u", episode);
    snprintf(objects_path, sizeof(objects_path), "OBJECTS.%u", episode);

    const int walls_ok =
        N3D_RE_LoadDefinitionTable(walls_path, &n3d_wall_definitions);
    const int objects_ok =
        N3D_RE_LoadDefinitionTable(objects_path, &n3d_object_definitions);

    N3D_RE_RebuildKnownMappedTypes();

    return walls_ok && objects_ok;
}
