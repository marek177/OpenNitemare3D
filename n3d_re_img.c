#include "n3d_re_img.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

n3d_img_archive n3d_img;

static uint16_t N3D_RE_ImgReadU16(
    const uint8_t* bytes)
{
    return (uint16_t)(
        (uint16_t)bytes[0] |
        ((uint16_t)bytes[1] << 8));
}

static uint32_t N3D_RE_ImgReadU32(
    const uint8_t* bytes)
{
    return
        (uint32_t)bytes[0] |
        ((uint32_t)bytes[1] << 8) |
        ((uint32_t)bytes[2] << 16) |
        ((uint32_t)bytes[3] << 24);
}

static int N3D_RE_ImgRange(
    const n3d_img_archive* archive,
    size_t offset,
    size_t length)
{
    return archive &&
           offset <= archive->size &&
           length <= archive->size - offset;
}

void N3D_RE_FreeImgArchive(n3d_img_archive* archive)
{
    if(!archive)
        return;

    free(archive->bytes);
    memset(archive, 0, sizeof(*archive));
}

static void N3D_RE_DecodeSequence(
    const uint8_t* src,
    n3d_img_sequence_def* dst)
{
    dst->interval_ms = N3D_RE_ImgReadU16(src);
    dst->frame_count = src[2];
    dst->extended = src[3];
    memcpy(dst->unknown, src + 4, sizeof(dst->unknown));
}

int N3D_RE_LoadImgArchive(
    const char* path,
    n3d_img_archive* archive)
{
    if(!path || !archive)
        return 0;

    FILE* file = fopen(path, "rb");
    if(!file)
        return 0;

    if(fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return 0;
    }

    const long end = ftell(file);
    if(end < 0 || (unsigned long)end < N3D_IMG_FRAME_DATA_OFFSET)
    {
        fclose(file);
        return 0;
    }

    if(fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return 0;
    }

    n3d_img_archive loaded;
    memset(&loaded, 0, sizeof(loaded));
    loaded.size = (size_t)end;
    loaded.bytes = (uint8_t*)malloc(loaded.size);
    if(!loaded.bytes)
    {
        fclose(file);
        return 0;
    }

    if(fread(loaded.bytes, 1, loaded.size, file) != loaded.size)
    {
        fclose(file);
        free(loaded.bytes);
        return 0;
    }
    fclose(file);

    for(size_t i = 0; i < N3D_IMG_INDEX_ENTRIES; ++i)
    {
        const size_t resource_dir =
            N3D_IMG_RESOURCE_DIRECTORY_OFFSET + i * 4u;

        /*
         * Win16 1.10 FUN_4C8A/4B86 and the wall/VEC loader both index the
         * same 256-entry DWORD resource directory at file offset 0.
         * The former 0x0400 "object directory" interpretation was incorrect.
         */
        const uint32_t resource_offset =
            N3D_RE_ImgReadU32(
                loaded.bytes + resource_dir);

        loaded.wall_offset[i] = resource_offset;
        loaded.object_offset[i] = resource_offset;

        if(loaded.wall_offset[i] != 0 &&
           (loaded.wall_offset[i] < N3D_IMG_FRAME_DATA_OFFSET ||
            loaded.wall_offset[i] >= loaded.size))
        {
            free(loaded.bytes);
            return 0;
        }

        if(loaded.object_offset[i] != 0 &&
           (loaded.object_offset[i] < N3D_IMG_FRAME_DATA_OFFSET ||
            loaded.object_offset[i] >= loaded.size))
        {
            free(loaded.bytes);
            return 0;
        }

        const size_t low =
            N3D_IMG_LOW_SEQUENCE_BANK_OFFSET +
            i * N3D_IMG_SEQUENCE_RECORD_BYTES;
        const size_t high =
            N3D_IMG_HIGH_SEQUENCE_BANK_OFFSET +
            i * N3D_IMG_SEQUENCE_RECORD_BYTES;

        if(low + N3D_IMG_SEQUENCE_RECORD_BYTES >
               N3D_IMG_FRAME_DATA_OFFSET ||
           high + N3D_IMG_SEQUENCE_RECORD_BYTES >
               N3D_IMG_FRAME_DATA_OFFSET)
        {
            free(loaded.bytes);
            return 0;
        }

        N3D_RE_DecodeSequence(
            loaded.bytes + low,
            &loaded.wall_sequence[i]);
        N3D_RE_DecodeSequence(
            loaded.bytes + high,
            &loaded.object_sequence[i]);
    }

    loaded.loaded = 1;

    N3D_RE_FreeImgArchive(archive);
    *archive = loaded;
    return 1;
}

int N3D_RE_LoadImgEpisode(uint8_t episode)
{
    char path[32];
    snprintf(path, sizeof(path), "IMG.%u", episode);
    return N3D_RE_LoadImgArchive(path, &n3d_img);
}

const n3d_img_sequence_def* N3D_RE_WallSequence(
    const n3d_img_archive* archive,
    uint8_t wall_id)
{
    if(!archive || !archive->loaded)
        return NULL;

    return &archive->wall_sequence[wall_id];
}

const n3d_img_sequence_def* N3D_RE_ObjectSequence(
    const n3d_img_archive* archive,
    uint8_t object_id)
{
    if(!archive || !archive->loaded)
        return NULL;

    return &archive->object_sequence[object_id];
}

static int N3D_RE_SequenceFrameFromOffset(
    const n3d_img_archive* archive,
    uint32_t stream_offset,
    unsigned frame_index,
    n3d_img_frame_view* out_frame)
{
    if(!archive || !archive->loaded || !out_frame ||
       stream_offset < N3D_IMG_FRAME_DATA_OFFSET)
        return 0;

    uint32_t current = stream_offset;

    for(unsigned i = 0; i <= frame_index; ++i)
    {
        if(!N3D_RE_ImgRange(
                archive,
                current,
                N3D_IMG_FRAME_HEADER_BYTES))
            return 0;

        const uint8_t* header = archive->bytes + current;
        const uint8_t width = header[0];
        const uint8_t height = header[1];
        const size_t pixel_count =
            (size_t)width * (size_t)height;
        const size_t full_size =
            N3D_IMG_FRAME_HEADER_BYTES + pixel_count;

        if(!N3D_RE_ImgRange(archive, current, full_size))
            return 0;

        if(i == frame_index)
        {
            memset(out_frame, 0, sizeof(*out_frame));
            out_frame->file_offset = current;
            out_frame->width = width;
            out_frame->height = height;
            memcpy(out_frame->metadata, header + 2, 8);
            out_frame->pixels =
                header + N3D_IMG_FRAME_HEADER_BYTES;
            out_frame->pixel_count = pixel_count;
            return 1;
        }

        const size_t next =
            (size_t)current + full_size;
        if(next > 0xFFFFFFFFu)
            return 0;

        current = (uint32_t)next;
    }

    return 0;
}

int N3D_RE_WallSequenceFrame(
    const n3d_img_archive* archive,
    uint8_t wall_id,
    unsigned frame_index,
    n3d_img_frame_view* out_frame)
{
    const n3d_img_sequence_def* sequence =
        N3D_RE_WallSequence(archive, wall_id);
    if(!sequence || frame_index >= sequence->frame_count)
        return 0;

    const uint32_t stream = archive->wall_offset[wall_id];
    if(stream == 0)
        return 0;

    return N3D_RE_SequenceFrameFromOffset(
        archive,
        stream,
        frame_index,
        out_frame);
}

int N3D_RE_ObjectSequenceFrame(
    const n3d_img_archive* archive,
    uint8_t object_id,
    unsigned frame_index,
    n3d_img_frame_view* out_frame)
{
    const n3d_img_sequence_def* sequence =
        N3D_RE_ObjectSequence(archive, object_id);
    if(!sequence || frame_index >= sequence->frame_count)
        return 0;

    const uint32_t stream = archive->object_offset[object_id];
    if(stream == 0)
        return 0;

    return N3D_RE_SequenceFrameFromOffset(
        archive,
        stream,
        frame_index,
        out_frame);
}
