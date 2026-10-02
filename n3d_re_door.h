#ifndef N3D_RE_DOOR_H
#define N3D_RE_DOOR_H

#include <stdint.h>

#define N3D_MAX_DOORS 64
#define N3D_DOOR_RECORD_SIZE 22
#define N3D_DOOR_STATE_OFFSET 0x0C
#define N3D_DOOR_TRANSITION_FLAG_OFFSET 0x14
#define N3D_DOOR_MOVE_STEP 2

typedef struct n3d_door_record
{
    /*
     * Full semantic field layout remains open. Keep bytes opaque and expose
     * only offsets directly supported by executable evidence.
     */
    uint8_t raw[N3D_DOOR_RECORD_SIZE];
} n3d_door_record;

/*
 * Modern bridge metadata. The original locates door records through a MAP-cell
 * far pointer stored somewhere in the still-partial 22-byte layout. Do not
 * invent that field: keep portable cell binding outside the opaque record.
 */
typedef struct n3d_door_binding
{
    uint8_t used;
    uint8_t cell_x;
    uint8_t cell_y;
    uint8_t state_known;
} n3d_door_binding;

_Static_assert(sizeof(n3d_door_record) == N3D_DOOR_RECORD_SIZE,
               "door runtime record must remain 22 bytes");

extern n3d_door_record n3d_doors[N3D_MAX_DOORS];
extern n3d_door_binding n3d_door_bindings[N3D_MAX_DOORS];
extern uint16_t n3d_door_count;

void N3D_RE_ResetDoors(void);
int N3D_RE_RegisterDoorCell(uint8_t x, uint8_t y);
int N3D_RE_FindDoorSlotByCell(uint8_t x, uint8_t y);
int N3D_RE_SetDoorCellState(uint8_t x, uint8_t y, uint8_t state);
int N3D_RE_DoorCellStateKnown(uint8_t x, uint8_t y);
int N3D_RE_DoorCellAllowsPassage(uint8_t x, uint8_t y);
int N3D_RE_DoorCellPassableCallback(uint8_t x, uint8_t y, void* user);

uint8_t N3D_RE_DoorStateValue(const n3d_door_record* door);
void N3D_RE_SetDoorStateValue(n3d_door_record* door, uint8_t state);

uint8_t N3D_RE_DoorTransitionFlag(const n3d_door_record* door);
void N3D_RE_SetDoorTransitionFlag(n3d_door_record* door, uint8_t value);

int N3D_RE_DoorRecordAllowsPassage(const n3d_door_record* door);

/*
 * Exact numeric command behavior recovered from remote-door menu paths.
 * Names "open"/"close" remain intentionally absent because that labeling is
 * inferred; use the original command IDs.
 */
int N3D_RE_ApplyRemoteDoorCommand(n3d_door_record* door, uint8_t command_id);

#endif
