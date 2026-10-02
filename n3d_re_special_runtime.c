#include "n3d_re_special_runtime.h"

#include <string.h>

n3d_panel_record n3d_panels[N3D_MAX_PANELS];
n3d_push_record n3d_pushes[N3D_MAX_PUSHES];
uint16_t n3d_panel_count;
uint16_t n3d_push_count;

void N3D_RE_ResetPanelsAndPushes(void)
{
    memset(n3d_panels, 0, sizeof(n3d_panels));
    memset(n3d_pushes, 0, sizeof(n3d_pushes));
    n3d_panel_count = 0;
    n3d_push_count = 0;
}

uint8_t N3D_RE_PanelActivation(const n3d_panel_record* panel)
{
    if(!panel)
        return 0;

    return panel->raw[N3D_PANEL_ACTIVATION_OFFSET];
}

void N3D_RE_SetPanelActivation(n3d_panel_record* panel, uint8_t value)
{
    if(!panel)
        return;

    panel->raw[N3D_PANEL_ACTIVATION_OFFSET] = value;
}

void N3D_RE_ActivatePanelUse(n3d_panel_record* panel)
{
    if(!panel)
        return;

    /* Direct USE path writes panel+0x14 = 2 and emits event 0x27 externally. */
    N3D_RE_SetPanelActivation(panel, 2);
}

n3d_push_direction N3D_RE_PushDirectionForOctant(uint8_t octant)
{
    static const int8_t dx[8] = {0, 8, 8, 0, 0, -8, -8, 0};
    static const int8_t dy[8] = {-8, 0, 0, 8, 8, 0, 0, -8};

    const uint8_t index = (uint8_t)(octant & 7u);
    n3d_push_direction direction = {dx[index], dy[index]};
    return direction;
}

int N3D_RE_CanStartPush(
    const n3d_push_record* push,
    uint8_t target_semantic_flags)
{
    if(!push)
        return 0;

    return push->steps_remaining == 0 &&
           (target_semantic_flags & 0x02u) == 0;
}

int N3D_RE_BeginPush(
    n3d_push_record* push,
    uint16_t object_index,
    uint8_t octant)
{
    if(!push || push->steps_remaining != 0)
        return 0;

    const n3d_push_direction direction =
        N3D_RE_PushDirectionForOctant(octant);

    push->object_index = object_index;
    push->delta_x = direction.dx;
    push->delta_y = direction.dy;
    push->steps_remaining = N3D_PUSH_STEPS;
    return 1;
}

int N3D_RE_StepPush(
    n3d_push_record* push,
    int8_t* dx,
    int8_t* dy)
{
    if(!push || push->steps_remaining == 0)
        return 0;

    if(dx) *dx = push->delta_x;
    if(dy) *dy = push->delta_y;

    --push->steps_remaining;
    return 1;
}
