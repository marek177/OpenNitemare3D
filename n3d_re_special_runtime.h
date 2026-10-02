#ifndef N3D_RE_SPECIAL_RUNTIME_H
#define N3D_RE_SPECIAL_RUNTIME_H

#include "n3d_re_runtime.h"

#include <stdint.h>

#define N3D_MAX_PANELS 32
#define N3D_PANEL_RECORD_SIZE 22
#define N3D_PANEL_ACTIVATION_OFFSET 0x14
#define N3D_PANEL_MAPPED_OBJECT_TYPE 0x03
#define N3D_PANEL_USE_EVENT 0x27
#define N3D_PANEL_MOVE_STEP 2

#define N3D_MAX_PUSHES 12
#define N3D_PUSH_RECORD_SIZE 6
#define N3D_PUSH_MAPPED_OBJECT_TYPE 0x28
#define N3D_PUSH_STEPS 8
#define N3D_PUSH_UNITS_PER_STEP 8

typedef struct n3d_panel_record
{
    uint8_t raw[N3D_PANEL_RECORD_SIZE];
} n3d_panel_record;

#pragma pack(push, 1)
typedef struct n3d_push_record
{
    uint16_t object_index;   /* +00 */
    int8_t delta_x;          /* +02 */
    int8_t delta_y;          /* +03 */
    uint8_t steps_remaining; /* +04 */
    uint8_t runtime_05;      /* +05 unresolved */
} n3d_push_record;
#pragma pack(pop)

_Static_assert(sizeof(n3d_panel_record) == N3D_PANEL_RECORD_SIZE,
               "panel runtime record must remain 22 bytes");
_Static_assert(sizeof(n3d_push_record) == N3D_PUSH_RECORD_SIZE,
               "push runtime record must remain 6 bytes");

typedef struct n3d_push_direction
{
    int8_t dx;
    int8_t dy;
} n3d_push_direction;

extern n3d_panel_record n3d_panels[N3D_MAX_PANELS];
extern n3d_push_record n3d_pushes[N3D_MAX_PUSHES];
extern uint16_t n3d_panel_count;
extern uint16_t n3d_push_count;

void N3D_RE_ResetPanelsAndPushes(void);

uint8_t N3D_RE_PanelActivation(const n3d_panel_record* panel);
void N3D_RE_SetPanelActivation(n3d_panel_record* panel, uint8_t value);
void N3D_RE_ActivatePanelUse(n3d_panel_record* panel);

n3d_push_direction N3D_RE_PushDirectionForOctant(uint8_t octant);
int N3D_RE_CanStartPush(
    const n3d_push_record* push,
    uint8_t target_semantic_flags);
int N3D_RE_BeginPush(
    n3d_push_record* push,
    uint16_t object_index,
    uint8_t octant);
int N3D_RE_StepPush(
    n3d_push_record* push,
    int8_t* dx,
    int8_t* dy);

#endif
