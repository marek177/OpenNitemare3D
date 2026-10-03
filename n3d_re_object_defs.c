#include "n3d_re_object_defs.h"

#include <string.h>

n3d_object_definition_slot
    n3d_object_definitions_runtime[N3D_MAX_OBJECT_DEFINITIONS];
uint16_t n3d_object_definition_count;
uint8_t n3d_object_definition_id_by_raw[256];
uint8_t n3d_object_definition_id_known[256];

static int N3D_RE_ObjectDefRange(
    const n3d_img_archive* archive,
    size_t offset,
    size_t length)
{
    return archive &&
           archive->loaded &&
           offset <= archive->size &&
           length <= archive->size - offset;
}

static uint16_t N3D_RE_ObjectDefReadU16(const uint8_t* p)
{
    return (uint16_t)(
        (uint16_t)p[0] |
        ((uint16_t)p[1] << 8));
}

void N3D_RE_ResetObjectDefinitions(void)
{
    memset(
        n3d_object_definitions_runtime,
        0,
        sizeof(n3d_object_definitions_runtime));
    memset(
        n3d_object_definition_id_by_raw,
        0,
        sizeof(n3d_object_definition_id_by_raw));
    memset(
        n3d_object_definition_id_known,
        0,
        sizeof(n3d_object_definition_id_known));
    n3d_object_definition_count = 0;
}

uint8_t N3D_RE_ObjectDefinitionFrameCount(
    const n3d_object_definition_header* header)
{
    return header ? header->raw[0x02] : 0;
}

uint16_t N3D_RE_ObjectDefinitionWord(
    const n3d_object_definition_header* header,
    unsigned offset)
{
    if(!header ||
       offset + 2u > N3D_OBJECT_RESOURCE_HEADER_BYTES)
        return 0;

    return N3D_RE_ObjectDefReadU16(
        header->raw + offset);
}

int N3D_RE_ReadObjectDefinitionHeader(
    const n3d_img_archive* archive,
    uint8_t object_id,
    uint32_t* source_key,
    n3d_object_definition_header* header)
{
    if(!archive ||
       !archive->loaded ||
       !source_key ||
       !header)
        return 0;

    /*
     * Win16 1.10: object resources use the shared directory at +0000, while
     * their 0x5A definition headers live in the second header bank at +6200.
     */
    const uint32_t key =
        archive->object_offset[object_id];

    if(key == 0 ||
       key < N3D_IMG_FRAME_DATA_OFFSET ||
       key >= archive->size)
        return 0;

    const size_t header_offset =
        N3D_IMG_HIGH_SEQUENCE_BANK_OFFSET +
        (size_t)object_id *
        N3D_OBJECT_RESOURCE_HEADER_BYTES;

    if(!N3D_RE_ObjectDefRange(
            archive,
            header_offset,
            N3D_OBJECT_RESOURCE_HEADER_BYTES))
        return 0;

    n3d_object_definition_header decoded;
    memcpy(
        decoded.raw,
        archive->bytes + header_offset,
        sizeof(decoded.raw));

    /*
     * FUN_4B86 validates/decodes frame entries. Reproduce the structural
     * checks here even though the C-rewrite currently needs only the header.
     */
    const unsigned frame_count =
        N3D_RE_ObjectDefinitionFrameCount(&decoded);
    size_t cursor = key;

    for(unsigned i = 0; i < frame_count; ++i)
    {
        if(!N3D_RE_ObjectDefRange(
                archive,
                cursor,
                N3D_OBJECT_RESOURCE_ENTRY_BYTES))
            return 0;

        const uint8_t width =
            archive->bytes[cursor + 0];
        const uint8_t height =
            archive->bytes[cursor + 1];
        const size_t pixel_bytes =
            (size_t)width * (size_t)height;

        /* Original non-tile object resource limit. */
        if(pixel_bytes > 0x0C00u)
            return 0;

        const size_t pixel_offset =
            cursor + N3D_OBJECT_RESOURCE_ENTRY_BYTES;

        if(!N3D_RE_ObjectDefRange(
                archive,
                pixel_offset,
                pixel_bytes))
            return 0;

        cursor =
            pixel_offset + pixel_bytes;
    }

    *source_key = key;
    *header = decoded;
    return 1;
}

int N3D_RE_RegisterMapObjectDefinition(
    uint8_t object_id,
    uint8_t* definition_id)
{
    if(definition_id)
        *definition_id = 0;

    if(object_id == 0 ||
       !n3d_img.loaded)
        return 0;

    if(n3d_object_definition_id_known[object_id])
    {
        if(definition_id)
            *definition_id =
                n3d_object_definition_id_by_raw[object_id];
        return 1;
    }

    uint32_t source_key = 0;
    n3d_object_definition_header header;

    if(!N3D_RE_ReadObjectDefinitionHeader(
            &n3d_img,
            object_id,
            &source_key,
            &header))
        return 0;

    /*
     * FUN_4C8A deduplicates per-level definitions by sourceKey before adding
     * the next 0x5A header/runtime frame table.
     */
    for(uint16_t i = 0;
        i < n3d_object_definition_count;
        ++i)
    {
        if(n3d_object_definitions_runtime[i].present &&
           n3d_object_definitions_runtime[i].source_key ==
               source_key)
        {
            n3d_object_definition_id_by_raw[object_id] =
                (uint8_t)i;
            n3d_object_definition_id_known[object_id] = 1;

            if(definition_id)
                *definition_id = (uint8_t)i;
            return 1;
        }
    }

    if(n3d_object_definition_count >=
       N3D_MAX_OBJECT_DEFINITIONS)
        return 0;

    const uint16_t slot =
        n3d_object_definition_count++;

    n3d_object_definition_slot* dst =
        &n3d_object_definitions_runtime[slot];

    memset(dst, 0, sizeof(*dst));
    dst->present = 1;
    dst->source_key = source_key;
    dst->source_object_id = object_id;
    dst->header = header;

    n3d_object_definition_id_by_raw[object_id] =
        (uint8_t)slot;
    n3d_object_definition_id_known[object_id] = 1;

    if(definition_id)
        *definition_id = (uint8_t)slot;

    return 1;
}

const n3d_object_definition_slot*
N3D_RE_ObjectDefinition(uint8_t definition_id)
{
    if(definition_id >= n3d_object_definition_count ||
       !n3d_object_definitions_runtime[definition_id].present)
        return NULL;

    return &n3d_object_definitions_runtime[definition_id];
}

uint16_t N3D_RE_ObjectDefinitionDirectional(
    const n3d_object_definition_header* header,
    unsigned bank,
    unsigned selector)
{
    if(bank > 2)
        return 0;

    const unsigned base =
        bank == 0
            ? N3D_OBJECT_DEF_DIRECTIONAL_A_OFFSET
            : bank == 1
                ? N3D_OBJECT_DEF_DIRECTIONAL_B_OFFSET
                : N3D_OBJECT_DEF_DIRECTIONAL_C_OFFSET;

    return N3D_RE_ObjectDefinitionWord(
        header,
        base + (selector & 7u) * 2u);
}

int N3D_RE_ObjectDefinitionGuardStateSequence(
    const n3d_object_definition_header* header,
    uint8_t guard_state,
    uint16_t* sequence)
{
    if(!header || !sequence)
        return 0;

    unsigned offset = 0;

    switch(guard_state)
    {
        case N3D_GUARD_STATE_02:
            offset = N3D_OBJECT_DEF_STATE02_OFFSET;
            break;

        case N3D_GUARD_STATE_03:
            offset = N3D_OBJECT_DEF_STATE03_OFFSET;
            break;

        case N3D_GUARD_STATE_04:
            offset = N3D_OBJECT_DEF_STATE04_OFFSET;
            break;

        default:
            return 0;
    }

    *sequence =
        N3D_RE_ObjectDefinitionWord(
            header,
            offset);
    return 1;
}

uint16_t N3D_RE_ObjectDefinitionReactionSequence(
    const n3d_object_definition_header* header,
    unsigned selector)
{
    return N3D_RE_ObjectDefinitionWord(
        header,
        N3D_OBJECT_DEF_REACTION_OFFSET +
            (selector & 7u) * 2u);
}

uint16_t N3D_RE_ObjectDefinitionDeathSequence(
    const n3d_object_definition_header* header,
    unsigned selector)
{
    return N3D_RE_ObjectDefinitionWord(
        header,
        N3D_OBJECT_DEF_DEATH_OFFSET +
            (selector & 7u) * 2u);
}

int N3D_RE_PackedSequenceHasFrames(
    uint16_t packed_sequence)
{
    return (packed_sequence & 0xFF00u) != 0;
}
