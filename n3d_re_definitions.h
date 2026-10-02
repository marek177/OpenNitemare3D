#ifndef N3D_RE_DEFINITIONS_H
#define N3D_RE_DEFINITIONS_H

#include <stdint.h>

#define N3D_DEFINITION_COUNT 256
#define N3D_DEF_VISUAL_CODE_MAX 32
#define N3D_DEF_IMAGE_NAME_MAX 64
#define N3D_DEF_CLASS_NAME_MAX 64
#define N3D_DEF_DESCRIPTION_MAX 256
#define N3D_MAPPED_TYPE_UNKNOWN 0xFF

typedef struct n3d_definition_record
{
    uint8_t present;
    uint8_t id;
    char visual_code[N3D_DEF_VISUAL_CODE_MAX];
    char image_name[N3D_DEF_IMAGE_NAME_MAX];
    char class_name[N3D_DEF_CLASS_NAME_MAX];
    char description[N3D_DEF_DESCRIPTION_MAX];
} n3d_definition_record;

typedef struct n3d_definition_table
{
    n3d_definition_record record[N3D_DEFINITION_COUNT];
    uint16_t count;
} n3d_definition_table;

extern n3d_definition_table n3d_wall_definitions;
extern n3d_definition_table n3d_object_definitions;
extern uint8_t n3d_wall_mapped_type[N3D_DEFINITION_COUNT];
extern uint8_t n3d_object_mapped_type[N3D_DEFINITION_COUNT];

void N3D_RE_ClearDefinitionTable(n3d_definition_table* table);
int N3D_RE_ParseDefinitionLine(
    const char* line,
    n3d_definition_record* out_record);
int N3D_RE_LoadDefinitionTable(
    const char* path,
    n3d_definition_table* table);

const n3d_definition_record* N3D_RE_FindDefinition(
    const n3d_definition_table* table,
    uint8_t id);

int N3D_RE_KnownObjectMappedTypeForClass(
    const char* class_name,
    uint8_t* mapped_type);

void N3D_RE_RebuildKnownMappedTypes(void);
int N3D_RE_LoadEpisodeDefinitions(uint8_t episode);

#endif
