#include "i_ibk.h"
#include "d_dat.h"
#include <string.h>
#include <stdio.h>
#include "i_sound.h"

#define I_IBKOFFSET 0

typedef struct ibkInstrument
{
    uint8_t iModChar;
    uint8_t iCarChar;
    uint8_t iModScale;
    uint8_t iCarScale;
    uint8_t iModAttack;
    uint8_t iCarAttack;
    uint8_t iModSustain;
    uint8_t iCarSustain;
    uint8_t iModWaveSel;
    uint8_t iCarWaveSel;
    uint8_t iFeedback;
    uint8_t iPercVoc;
    int8_t iTransPos; //trans rights, Mr Freeman
    int8_t iDPitch;
    uint8_t padding[2];
}ibkInstrument;

void I_LoadIBK()
{
    dat_entry_t entry = SND.entries[I_IBKOFFSET];

    void* data = D_GetData(entry);

    static char magic[] = {'I','B','K',0x1A};

    ibkInstrument instruments[128];
    const char* names[128];

    //todo: do magic checksum lol
    data += 4;
    memcpy(instruments, data, sizeof(ibkInstrument) * 128);
    data += sizeof(ibkInstrument) * 128;

    for(int i = 0; i < 128; i++)
    {
        // memcpy(names[i], data, 9);
        names[i] = data;
        data += 9;
    }

    for(int i = 0; i < 128; i++)
        printf("instrument[%d]: %s\n", i, names[i]);

    fluid_sfont_t* font =  I_GetSoundFont();
}