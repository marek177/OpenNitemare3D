#ifndef N3D_RE_RUNTIME_H
#define N3D_RE_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

#define N3D_MAP_WIDTH 64
#define N3D_MAP_HEIGHT 64
#define N3D_MAP_HEADER_BYTES 514
#define N3D_MAP_CELL_BYTES 2
#define N3D_MAP_LEVEL_BYTES (N3D_MAP_WIDTH * N3D_MAP_HEIGHT * N3D_MAP_CELL_BYTES)

#define N3D_WORLD_UNITS_PER_TILE 64
#define N3D_TILE_CENTER_OFFSET 32
#define N3D_PLAYER_COLLISION_HALF_EXTENT 27

#define N3D_MAX_OBJECTS 350
#define N3D_OBJECT_STRIDE 28
#define N3D_MAX_GUARDS 100
#define N3D_GUARD_STRIDE 26
#define N3D_MAX_PROJECTILES 8
#define N3D_PROJECTILE_STRIDE 42

#define N3D_OBJECT_RUNTIME_PRESENT 0x01
#define N3D_OBJECT_BLOCKS_MOVEMENT 0x02
#define N3D_OBJECT_SPECIAL_TOUCH 0x04
#define N3D_OBJECT_CREATES_GUARD 0x08

typedef struct n3d_map_cell
{
    uint8_t wall;
    uint8_t object;
} n3d_map_cell;

/*
 * Portable binding metadata. Original OBJECT+0x0C/+0x0E is a Win16 far
 * pointer to the MAP cell; do not forge a segment value in reconstructed C.
 */
typedef struct n3d_object_binding
{
    uint8_t used;
    uint8_t cell_x;
    uint8_t cell_y;
} n3d_object_binding;

#pragma pack(push, 1)
typedef struct n3d_object_record
{
    uint8_t map_object_id;       /* +00 */
    uint8_t variant;             /* +01 */
    int8_t animation_aux;        /* +02 */
    int8_t animation_frame;      /* +03 */
    uint8_t sequence_id;         /* +04 ordinary OBJECT definition id; projectile sequence id */
    uint8_t flags;               /* +05 */
    uint8_t object_class;        /* +06 */
    uint8_t guard_index;         /* +07 */
    uint32_t animation_deadline; /* +08 */
    uint32_t map_cell_binding;   /* +0C modern placeholder for original far pointer */
    int16_t world_x;             /* +10 */
    int16_t world_y;             /* +12 */
    int16_t render_sort_a;       /* +14 */
    int16_t render_sort_b;       /* +16 */
    int16_t projected_y_base;    /* +18 */
    uint8_t runtime_1a;          /* +1A */
    uint8_t unknown_1b;          /* +1B */
} n3d_object_record;

typedef struct n3d_guard_record
{
    uint16_t definition_value; /* +00 */
    uint32_t timestamp;        /* +02 */
    int16_t timer;             /* +06 */
    uint16_t object_slot;      /* +08 */
    uint8_t strategy;          /* +0A */
    uint8_t state;             /* +0B */
    uint8_t next_state;        /* +0C */
    uint8_t saved_map_object;  /* +0D */
    uint8_t area_id;           /* +0E persistent class-0x44 AREA id; starts FF */
    uint8_t control;           /* +0F partial semantic */
    uint8_t strength;          /* +10 */
    uint8_t octant;            /* +11 */
    uint8_t result_octant;     /* +12 */
    int8_t move_x;             /* +13 */
    int8_t move_y;             /* +14 */
    uint8_t unknown_15;        /* +15 */
    uint8_t transition_flag;   /* +16 transition/perception control: 0 proximity, 1/2 LOS */
    uint8_t unknown_17;        /* +17 cached perception result */
    uint8_t unknown_18;        /* +18 cached <=1-tile proximity */
    uint8_t unknown_19;
} n3d_guard_record;

typedef struct n3d_projectile_record
{
    int16_t x_is_major_axis;    /* +00 */
    int16_t line_error;         /* +02 */
    int16_t minor_error_step;   /* +04 */
    int16_t major_error_fixup;  /* +06 */
    int16_t step_x;             /* +08 */
    int16_t step_y;             /* +0A */
    uint8_t state;              /* +0C: 0 free, 1 flying, 2 impact */
    uint8_t unknown_0d;         /* +0D */
    n3d_object_record object;   /* +0E */
} n3d_projectile_record;
#pragma pack(pop)

_Static_assert(sizeof(n3d_object_record) == N3D_OBJECT_STRIDE, "OBJECT layout must be 28 bytes");
_Static_assert(sizeof(n3d_guard_record) == N3D_GUARD_STRIDE, "GUARD layout must be 26 bytes");
_Static_assert(sizeof(n3d_projectile_record) == N3D_PROJECTILE_STRIDE, "projectile layout must be 42 bytes");
_Static_assert(offsetof(n3d_object_record, world_x) == 0x10, "OBJECT world_x offset");
_Static_assert(offsetof(n3d_object_record, world_y) == 0x12, "OBJECT world_y offset");
_Static_assert(offsetof(n3d_guard_record, object_slot) == 0x08, "GUARD object slot offset");
_Static_assert(offsetof(n3d_guard_record, state) == 0x0B, "GUARD state offset");
_Static_assert(offsetof(n3d_guard_record, strength) == 0x10, "GUARD strength offset");
_Static_assert(offsetof(n3d_projectile_record, object) == 0x0E, "projectile embedded OBJECT offset");

typedef enum n3d_guard_state
{
    N3D_GUARD_STATE_00 = 0x00,
    N3D_GUARD_STATE_01 = 0x01,
    N3D_GUARD_STATE_02 = 0x02,
    N3D_GUARD_STATE_03 = 0x03,
    N3D_GUARD_STATE_04 = 0x04,
    N3D_GUARD_STATE_05 = 0x05,
    N3D_GUARD_STATE_06 = 0x06,
    N3D_GUARD_STATE_07 = 0x07,
    N3D_GUARD_STATE_08 = 0x08,
    N3D_GUARD_STATE_09 = 0x09,
    N3D_GUARD_STATE_0A = 0x0A,
    N3D_GUARD_STATE_LETHAL_CONTACT = 0x0B,
    N3D_GUARD_STATE_0C = 0x0C,
    N3D_GUARD_STATE_0D = 0x0D,
    N3D_GUARD_STATE_0E = 0x0E,
    N3D_GUARD_STATE_0F = 0x0F,
    N3D_GUARD_STATE_10 = 0x10,
    N3D_GUARD_STATE_11 = 0x11,
    N3D_GUARD_STATE_12 = 0x12,
    N3D_GUARD_STATE_TIMED_DIRECTIONAL_MOVE = 0x13,
    N3D_GUARD_STATE_14 = 0x14,
    N3D_GUARD_STATE_PAIN = 0x15
} n3d_guard_state;

typedef struct n3d_guard_initial_profile
{
    uint8_t strategy;
    uint8_t state;
    uint8_t next_state;
    uint8_t perception_mode;
} n3d_guard_initial_profile;

extern n3d_map_cell n3d_map[N3D_MAP_WIDTH * N3D_MAP_HEIGHT];
extern n3d_object_record n3d_objects[N3D_MAX_OBJECTS];
extern n3d_object_binding n3d_object_bindings[N3D_MAX_OBJECTS];
extern n3d_guard_record n3d_guards[N3D_MAX_GUARDS];
extern n3d_projectile_record n3d_projectiles[N3D_MAX_PROJECTILES];
extern uint16_t n3d_object_count;
extern uint16_t n3d_guard_count;

void N3D_RE_ResetRuntime(void);
int N3D_RE_LoadMapPayload(const uint8_t* payload, size_t payload_size);
const n3d_map_cell* N3D_RE_MapCell(uint8_t x, uint8_t y);
int N3D_RE_FindObjectSlotByCell(uint8_t x, uint8_t y);
int N3D_RE_InstantiateMapObject(
    uint8_t map_object_id,
    uint8_t tile_x,
    uint8_t tile_y,
    int* object_slot);
int N3D_RE_GuardClassFromMapObject(uint8_t map_object_id, uint8_t* object_class);
n3d_guard_initial_profile N3D_RE_GuardInitialProfile(uint8_t object_class);
int N3D_RE_RegisterGuardFromMap(uint8_t map_object_id, uint8_t tile_x, uint8_t tile_y);

#endif
