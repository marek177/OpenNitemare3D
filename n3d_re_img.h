#ifndef N3D_RE_IMG_H
#define N3D_RE_IMG_H

#include <stddef.h>
#include <stdint.h>

#define N3D_IMG_INDEX_ENTRIES 256
#define N3D_IMG_RESOURCE_DIRECTORY_OFFSET 0x0000u
#define N3D_IMG_WALL_DIRECTORY_OFFSET N3D_IMG_RESOURCE_DIRECTORY_OFFSET
#define N3D_IMG_OBJECT_DIRECTORY_OFFSET N3D_IMG_RESOURCE_DIRECTORY_OFFSET
#define N3D_IMG_LOW_SEQUENCE_BANK_OFFSET 0x0800u
#define N3D_IMG_HIGH_SEQUENCE_BANK_OFFSET 0x6200u
#define N3D_IMG_SEQUENCE_RECORD_BYTES 90u
#define N3D_IMG_FRAME_HEADER_BYTES 10u
#define N3D_IMG_FRAME_DATA_OFFSET 0xBC00u

#pragma pack(push, 1)
typedef struct n3d_img_sequence_def
{
    uint16_t interval_ms;  /* +00 */
    uint8_t frame_count;   /* +02 */
    uint8_t extended;      /* +03 */
    uint8_t unknown[86];   /* +04..+59 */
} n3d_img_sequence_def;
#pragma pack(pop)

_Static_assert(sizeof(n3d_img_sequence_def) == N3D_IMG_SEQUENCE_RECORD_BYTES,
               "IMG SEQDEF record must remain 90 bytes");

typedef struct n3d_img_frame_view
{
    uint32_t file_offset;
    uint8_t width;
    uint8_t height;
    uint8_t metadata[8];
    const uint8_t* pixels;
    size_t pixel_count;
} n3d_img_frame_view;

typedef struct n3d_img_archive
{
    uint8_t* bytes;
    size_t size;
    uint32_t wall_offset[N3D_IMG_INDEX_ENTRIES];
    uint32_t object_offset[N3D_IMG_INDEX_ENTRIES];
    n3d_img_sequence_def wall_sequence[N3D_IMG_INDEX_ENTRIES];
    n3d_img_sequence_def object_sequence[N3D_IMG_INDEX_ENTRIES];
    uint8_t loaded;
} n3d_img_archive;

extern n3d_img_archive n3d_img;

void N3D_RE_FreeImgArchive(n3d_img_archive* archive);
int N3D_RE_LoadImgArchive(const char* path, n3d_img_archive* archive);
int N3D_RE_LoadImgEpisode(uint8_t episode);

const n3d_img_sequence_def* N3D_RE_WallSequence(
    const n3d_img_archive* archive,
    uint8_t wall_id);

const n3d_img_sequence_def* N3D_RE_ObjectSequence(
    const n3d_img_archive* archive,
    uint8_t object_id);

int N3D_RE_WallSequenceFrame(
    const n3d_img_archive* archive,
    uint8_t wall_id,
    unsigned frame_index,
    n3d_img_frame_view* out_frame);

int N3D_RE_ObjectSequenceFrame(
    const n3d_img_archive* archive,
    uint8_t object_id,
    unsigned frame_index,
    n3d_img_frame_view* out_frame);

#endif
