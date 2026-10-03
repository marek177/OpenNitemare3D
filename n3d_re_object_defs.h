#ifndef N3D_RE_OBJECT_DEFS_H
#define N3D_RE_OBJECT_DEFS_H

#include "n3d_re_img.h"
#include "n3d_re_runtime.h"

#include <stdint.h>

#define N3D_OBJECT_RESOURCE_HEADER_BYTES 0x5Au
#define N3D_OBJECT_RESOURCE_ENTRY_BYTES 10u
#define N3D_MAX_OBJECT_DEFINITIONS 256u

#define N3D_OBJECT_DEF_DIRECTIONAL_A_OFFSET 0x04u
#define N3D_OBJECT_DEF_DIRECTIONAL_B_OFFSET 0x14u
#define N3D_OBJECT_DEF_DIRECTIONAL_C_OFFSET 0x24u
#define N3D_OBJECT_DEF_STATE02_OFFSET 0x34u
#define N3D_OBJECT_DEF_STATE03_OFFSET 0x36u
#define N3D_OBJECT_DEF_STATE04_OFFSET 0x38u
#define N3D_OBJECT_DEF_REACTION_OFFSET 0x3Au
#define N3D_OBJECT_DEF_DEATH_OFFSET 0x4Au

typedef struct n3d_object_definition_header
{
    uint8_t raw[N3D_OBJECT_RESOURCE_HEADER_BYTES];
} n3d_object_definition_header;

typedef struct n3d_object_definition_slot
{
    uint8_t present;
    uint32_t source_key;
    uint8_t source_object_id;
    n3d_object_definition_header header;
} n3d_object_definition_slot;

extern n3d_object_definition_slot
    n3d_object_definitions_runtime[N3D_MAX_OBJECT_DEFINITIONS];
extern uint16_t n3d_object_definition_count;
extern uint8_t n3d_object_definition_id_by_raw[256];
extern uint8_t n3d_object_definition_id_known[256];

void N3D_RE_ResetObjectDefinitions(void);

int N3D_RE_ReadObjectDefinitionHeader(
    const n3d_img_archive* archive,
    uint8_t object_id,
    uint32_t* source_key,
    n3d_object_definition_header* header);

int N3D_RE_RegisterMapObjectDefinition(
    uint8_t object_id,
    uint8_t* definition_id);

const n3d_object_definition_slot*
N3D_RE_ObjectDefinition(uint8_t definition_id);

uint8_t N3D_RE_ObjectDefinitionFrameCount(
    const n3d_object_definition_header* header);

uint16_t N3D_RE_ObjectDefinitionWord(
    const n3d_object_definition_header* header,
    unsigned offset);

uint16_t N3D_RE_ObjectDefinitionDirectional(
    const n3d_object_definition_header* header,
    unsigned bank,
    unsigned selector);

int N3D_RE_ObjectDefinitionGuardStateSequence(
    const n3d_object_definition_header* header,
    uint8_t guard_state,
    uint16_t* sequence);

uint16_t N3D_RE_ObjectDefinitionReactionSequence(
    const n3d_object_definition_header* header,
    unsigned selector);

uint16_t N3D_RE_ObjectDefinitionDeathSequence(
    const n3d_object_definition_header* header,
    unsigned selector);

int N3D_RE_PackedSequenceHasFrames(uint16_t packed_sequence);

#endif
