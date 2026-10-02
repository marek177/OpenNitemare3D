#include "n3d_re_door.h"

#include <string.h>

n3d_door_record n3d_doors[N3D_MAX_DOORS];
uint16_t n3d_door_count;

void N3D_RE_ResetDoors(void)
{
    memset(n3d_doors, 0, sizeof(n3d_doors));
    n3d_door_count = 0;
}

uint8_t N3D_RE_DoorStateValue(const n3d_door_record* door)
{
    if(!door)
        return 0xFF;

    /*
     * Values 0..4 are observed at +0x0C. Full original field width remains
     * part of the unresolved 22-byte layout, so this accessor intentionally
     * exposes only the evidenced low byte.
     */
    return door->raw[N3D_DOOR_STATE_OFFSET];
}

void N3D_RE_SetDoorStateValue(n3d_door_record* door, uint8_t state)
{
    if(!door)
        return;

    door->raw[N3D_DOOR_STATE_OFFSET] = state;
}

uint8_t N3D_RE_DoorTransitionFlag(const n3d_door_record* door)
{
    if(!door)
        return 0;

    return door->raw[N3D_DOOR_TRANSITION_FLAG_OFFSET];
}

void N3D_RE_SetDoorTransitionFlag(n3d_door_record* door, uint8_t value)
{
    if(!door)
        return;

    door->raw[N3D_DOOR_TRANSITION_FLAG_OFFSET] = value;
}

int N3D_RE_DoorRecordAllowsPassage(const n3d_door_record* door)
{
    if(!door)
        return 0;

    const uint8_t state = N3D_RE_DoorStateValue(door);
    return state == 0 || state == 4;
}

int N3D_RE_ApplyRemoteDoorCommand(n3d_door_record* door, uint8_t command_id)
{
    if(!door)
        return 0;

    const uint8_t state = N3D_RE_DoorStateValue(door);

    if(command_id == 0x1E)
    {
        if(state != 1 && state != 3)
            return 0;

        N3D_RE_SetDoorTransitionFlag(door, 1);
        N3D_RE_SetDoorStateValue(door, 2);
        return 1;
    }

    if(command_id == 0x1F)
    {
        if(state != 0 && state != 2)
            return 0;

        N3D_RE_SetDoorTransitionFlag(door, 1);
        N3D_RE_SetDoorStateValue(door, 3);
        return 1;
    }

    return 0;
}
