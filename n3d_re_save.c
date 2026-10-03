#include "n3d_re_save.h"

#include <string.h>

static uint16_t N3D_RE_ReadU16(const uint8_t* p)
{
    return (uint16_t)(
        (uint16_t)p[0] |
        ((uint16_t)p[1] << 8));
}

static int16_t N3D_RE_ReadS16(const uint8_t* p)
{
    return (int16_t)N3D_RE_ReadU16(p);
}

static uint32_t N3D_RE_ReadU32(const uint8_t* p)
{
    return
        (uint32_t)N3D_RE_ReadU16(p) |
        ((uint32_t)N3D_RE_ReadU16(p + 2) << 16);
}

static void N3D_RE_WriteU16(uint8_t* p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void N3D_RE_WriteS16(uint8_t* p, int16_t value)
{
    N3D_RE_WriteU16(p, (uint16_t)value);
}

static void N3D_RE_WriteU32(uint8_t* p, uint32_t value)
{
    N3D_RE_WriteU16(p, (uint16_t)value);
    N3D_RE_WriteU16(p + 2, (uint16_t)(value >> 16));
}

static void N3D_RE_DecodeObject(
    const uint8_t* src,
    n3d_object_record* dst)
{
    dst->map_object_id = src[0x00];
    dst->variant = src[0x01];
    dst->animation_aux = (int8_t)src[0x02];
    dst->animation_frame = (int8_t)src[0x03];
    dst->sequence_id = src[0x04];
    dst->flags = src[0x05];
    dst->object_class = src[0x06];
    dst->guard_index = src[0x07];
    dst->animation_deadline = N3D_RE_ReadU32(src + 0x08);
    dst->map_cell_binding = N3D_RE_ReadU32(src + 0x0C);
    dst->world_x = N3D_RE_ReadS16(src + 0x10);
    dst->world_y = N3D_RE_ReadS16(src + 0x12);
    dst->render_sort_a = N3D_RE_ReadS16(src + 0x14);
    dst->render_sort_b = N3D_RE_ReadS16(src + 0x16);
    dst->projected_y_base = N3D_RE_ReadS16(src + 0x18);
    dst->runtime_1a = src[0x1A];
    dst->unknown_1b = src[0x1B];
}

static void N3D_RE_EncodeObject(
    uint8_t* dst,
    const n3d_object_record* src)
{
    dst[0x00] = src->map_object_id;
    dst[0x01] = src->variant;
    dst[0x02] = (uint8_t)src->animation_aux;
    dst[0x03] = (uint8_t)src->animation_frame;
    dst[0x04] = src->sequence_id;
    dst[0x05] = src->flags;
    dst[0x06] = src->object_class;
    dst[0x07] = src->guard_index;
    N3D_RE_WriteU32(dst + 0x08, src->animation_deadline);
    N3D_RE_WriteU32(dst + 0x0C, src->map_cell_binding);
    N3D_RE_WriteS16(dst + 0x10, src->world_x);
    N3D_RE_WriteS16(dst + 0x12, src->world_y);
    N3D_RE_WriteS16(dst + 0x14, src->render_sort_a);
    N3D_RE_WriteS16(dst + 0x16, src->render_sort_b);
    N3D_RE_WriteS16(dst + 0x18, src->projected_y_base);
    dst[0x1A] = src->runtime_1a;
    dst[0x1B] = src->unknown_1b;
}

static void N3D_RE_DecodeGuard(
    const uint8_t* src,
    n3d_guard_record* dst)
{
    dst->definition_value = N3D_RE_ReadU16(src + 0x00);
    dst->timestamp = N3D_RE_ReadU32(src + 0x02);
    dst->timer = N3D_RE_ReadS16(src + 0x06);
    dst->object_slot = N3D_RE_ReadU16(src + 0x08);
    dst->strategy = src[0x0A];
    dst->state = src[0x0B];
    dst->next_state = src[0x0C];
    dst->saved_map_object = src[0x0D];
    dst->area_id = src[0x0E];
    dst->control = src[0x0F];
    dst->strength = src[0x10];
    dst->octant = src[0x11];
    dst->result_octant = src[0x12];
    dst->move_x = (int8_t)src[0x13];
    dst->move_y = (int8_t)src[0x14];
    dst->unknown_15 = src[0x15];
    dst->transition_flag = src[0x16];
    dst->unknown_17 = src[0x17];
    dst->unknown_18 = src[0x18];
    dst->unknown_19 = src[0x19];
}

static void N3D_RE_EncodeGuard(
    uint8_t* dst,
    const n3d_guard_record* src)
{
    N3D_RE_WriteU16(dst + 0x00, src->definition_value);
    N3D_RE_WriteU32(dst + 0x02, src->timestamp);
    N3D_RE_WriteS16(dst + 0x06, src->timer);
    N3D_RE_WriteU16(dst + 0x08, src->object_slot);
    dst[0x0A] = src->strategy;
    dst[0x0B] = src->state;
    dst[0x0C] = src->next_state;
    dst[0x0D] = src->saved_map_object;
    dst[0x0E] = src->area_id;
    dst[0x0F] = src->control;
    dst[0x10] = src->strength;
    dst[0x11] = src->octant;
    dst[0x12] = src->result_octant;
    dst[0x13] = (uint8_t)src->move_x;
    dst[0x14] = (uint8_t)src->move_y;
    dst[0x15] = src->unknown_15;
    dst[0x16] = src->transition_flag;
    dst[0x17] = src->unknown_17;
    dst[0x18] = src->unknown_18;
    dst[0x19] = src->unknown_19;
}

static void N3D_RE_DecodeProjectile(
    const uint8_t* src,
    n3d_projectile_record* dst)
{
    dst->x_is_major_axis = N3D_RE_ReadS16(src + 0x00);
    dst->line_error = N3D_RE_ReadS16(src + 0x02);
    dst->minor_error_step = N3D_RE_ReadS16(src + 0x04);
    dst->major_error_fixup = N3D_RE_ReadS16(src + 0x06);
    dst->step_x = N3D_RE_ReadS16(src + 0x08);
    dst->step_y = N3D_RE_ReadS16(src + 0x0A);
    dst->state = src[0x0C];
    dst->unknown_0d = src[0x0D];
    N3D_RE_DecodeObject(src + 0x0E, &dst->object);
}

static void N3D_RE_EncodeProjectile(
    uint8_t* dst,
    const n3d_projectile_record* src)
{
    N3D_RE_WriteS16(dst + 0x00, src->x_is_major_axis);
    N3D_RE_WriteS16(dst + 0x02, src->line_error);
    N3D_RE_WriteS16(dst + 0x04, src->minor_error_step);
    N3D_RE_WriteS16(dst + 0x06, src->major_error_fixup);
    N3D_RE_WriteS16(dst + 0x08, src->step_x);
    N3D_RE_WriteS16(dst + 0x0A, src->step_y);
    dst[0x0C] = src->state;
    dst[0x0D] = src->unknown_0d;
    N3D_RE_EncodeObject(dst + 0x0E, &src->object);
}

static void N3D_RE_DecodePush(
    const uint8_t* src,
    n3d_push_record* dst)
{
    dst->object_index = N3D_RE_ReadU16(src + 0x00);
    dst->delta_x = (int8_t)src[0x02];
    dst->delta_y = (int8_t)src[0x03];
    dst->steps_remaining = src[0x04];
    dst->runtime_05 = src[0x05];
}

static void N3D_RE_EncodePush(
    uint8_t* dst,
    const n3d_push_record* src)
{
    N3D_RE_WriteU16(dst + 0x00, src->object_index);
    dst[0x02] = (uint8_t)src->delta_x;
    dst[0x03] = (uint8_t)src->delta_y;
    dst[0x04] = src->steps_remaining;
    dst[0x05] = src->runtime_05;
}

int N3D_RE_ReadKnownSaveBlocks(
    const uint8_t* slot,
    size_t slot_size,
    n3d_known_save_blocks* out_blocks)
{
    if(!slot || !out_blocks || slot_size < N3D_USER_SAVE_SLOT_BYTES)
        return 0;

    memset(out_blocks, 0, sizeof(*out_blocks));

    for(size_t i = 0; i < N3D_MAX_OBJECTS; ++i)
        N3D_RE_DecodeObject(
            slot + N3D_SAVE_OBJECT_OFFSET + i * N3D_OBJECT_STRIDE,
            &out_blocks->objects[i]);

    for(size_t i = 0; i < N3D_MAX_GUARDS; ++i)
        N3D_RE_DecodeGuard(
            slot + N3D_SAVE_GUARD_OFFSET + i * N3D_GUARD_STRIDE,
            &out_blocks->guards[i]);

    memcpy(out_blocks->doors,
           slot + N3D_SAVE_DOOR_OFFSET,
           N3D_SAVE_DOOR_BYTES);

    memcpy(out_blocks->panel_activation,
           slot + N3D_SAVE_PANEL_ACTIVATION_OFFSET,
           N3D_SAVE_PANEL_ACTIVATION_BYTES);

    for(size_t i = 0; i < N3D_MAX_PROJECTILES; ++i)
        N3D_RE_DecodeProjectile(
            slot + N3D_SAVE_PROJECTILE_OFFSET + i * N3D_PROJECTILE_STRIDE,
            &out_blocks->projectiles[i]);

    memcpy(out_blocks->global_51a4,
           slot + N3D_SAVE_GLOBAL_51A4_OFFSET,
           N3D_SAVE_GLOBAL_51A4_BYTES);

    for(size_t i = 0; i < N3D_MAX_PUSHES; ++i)
        N3D_RE_DecodePush(
            slot + N3D_SAVE_PUSH_OFFSET + i * N3D_PUSH_RECORD_SIZE,
            &out_blocks->pushes[i]);

    memcpy(out_blocks->guard_wake_cache,
           slot + N3D_SAVE_GUARD_WAKE_OFFSET,
           N3D_SAVE_GUARD_WAKE_BYTES);

    return 1;
}

int N3D_RE_WriteKnownSaveBlocks(
    uint8_t* slot,
    size_t slot_size,
    const n3d_known_save_blocks* blocks)
{
    if(!slot || !blocks || slot_size < N3D_USER_SAVE_SLOT_BYTES)
        return 0;

    for(size_t i = 0; i < N3D_MAX_OBJECTS; ++i)
        N3D_RE_EncodeObject(
            slot + N3D_SAVE_OBJECT_OFFSET + i * N3D_OBJECT_STRIDE,
            &blocks->objects[i]);

    for(size_t i = 0; i < N3D_MAX_GUARDS; ++i)
        N3D_RE_EncodeGuard(
            slot + N3D_SAVE_GUARD_OFFSET + i * N3D_GUARD_STRIDE,
            &blocks->guards[i]);

    memcpy(slot + N3D_SAVE_DOOR_OFFSET,
           blocks->doors,
           N3D_SAVE_DOOR_BYTES);

    memcpy(slot + N3D_SAVE_PANEL_ACTIVATION_OFFSET,
           blocks->panel_activation,
           N3D_SAVE_PANEL_ACTIVATION_BYTES);

    for(size_t i = 0; i < N3D_MAX_PROJECTILES; ++i)
        N3D_RE_EncodeProjectile(
            slot + N3D_SAVE_PROJECTILE_OFFSET + i * N3D_PROJECTILE_STRIDE,
            &blocks->projectiles[i]);

    memcpy(slot + N3D_SAVE_GLOBAL_51A4_OFFSET,
           blocks->global_51a4,
           N3D_SAVE_GLOBAL_51A4_BYTES);

    for(size_t i = 0; i < N3D_MAX_PUSHES; ++i)
        N3D_RE_EncodePush(
            slot + N3D_SAVE_PUSH_OFFSET + i * N3D_PUSH_RECORD_SIZE,
            &blocks->pushes[i]);

    memcpy(slot + N3D_SAVE_GUARD_WAKE_OFFSET,
           blocks->guard_wake_cache,
           N3D_SAVE_GUARD_WAKE_BYTES);

    return 1;
}
