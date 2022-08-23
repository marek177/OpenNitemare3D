#include "d_dat.h"


dat_file_t UIF, SND;

void D_LoadDats()
{
    SND = D_ReadDat("SND.DAT");
    UIF = D_ReadDat("UIF.DAT");
}

void D_FreeEntry(dat_entry_t entry)
{
    free(entry.data);
}

void D_FreeDat(dat_file_t* file)
{
    for(int i = 0; i < file->count; i++)
    {
        free(file->entries[i].data);
    }
}


void* D_GetData(dat_entry_t entry)
{
    if(1)
    {
        FILE* file = fopen(entry.filename, "rb");
        fseek(file, entry.offset, SEEK_SET);
        fread(entry.data, entry.length, 1, file);
        fclose(file);
        // entry.data = buffer;
    }
    return entry.data;
}

dat_file_t D_ReadDat(char filename[256])
{
    dat_file_t dat;
    
    uint32_t* offsets = malloc(sizeof(uint32_t) * 1000);
    uint16_t* lengths = malloc(sizeof(uint16_t) * 1000);

    FILE* file = fopen(filename, "rb");

    int count = 0;

    fseek(file, 0L, SEEK_END);
    long filelen = ftell(file);
    fseek(file, 0L, SEEK_SET);
    while(1)
    {
        uint32_t offset;
        uint16_t length;

        fread(&length, 2, 1, file);
        fread(&offset, 4, 1, file);

        lengths[count] = length;
        offsets[count] = offset;
        count++;

        if (offset + length == filelen)
            break;

    }

    dat_entry_t* entries = malloc(sizeof(dat_entry_t) * count);
    for(int i = 0; i < count; i++)
    {
        uint16_t length = lengths[i];
        uint32_t offset = offsets[i];
        fseek(file, offset, SEEK_SET);
        byte* buffer = malloc(length);
        fread(buffer, length, 1, file);

        dat_entry_t entry;

        // entry.data = buffer;
        entry.length = length;
        entry.filename = filename;
        entry.offset = offset;
        entry.data = malloc(entry.length);
        entries[i] = entry;
        

        char strBuffer[64];
        sprintf(strBuffer, "DAT/%s/%d.BIN", filename, i);

        I_DumpBinary(buffer, entry.length, strBuffer);
        free(buffer);
        // char* str = malloc(256);
        // sprintf(str, "dat/%s/%d.bin", filename, i);
        // puts(str);
        // I_DumpBinary(buffer, length, str);
    }
    dat.entries = entries;
    dat.count = count;
    fclose(file);
    free(offsets);
    free(lengths);
    return dat;
}