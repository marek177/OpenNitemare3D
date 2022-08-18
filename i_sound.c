#include "i_sound.h"
Mix_Music* current_midi;

void I_InitMusic()
{
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    I_SetVolume(10);
}

void I_SetVolume(byte volume)
{
    Mix_VolumeMusic(volume);
}


void I_PauseMusic()
{
    Mix_PauseMusic();
}

void I_PlayMusic()
{
    Mix_PlayMusic(current_midi, -1);
}

void I_ChangeSong(int id)
{
    Mix_FreeMusic(current_midi);
    SDL_RWops* data = SDL_RWFromMem(D_GetData(SND.entries[id+1]), SND.entries[+1].length);
    current_midi = Mix_LoadMUS_RW(data, 1);
    printf("midi:%d\n", current_midi);
    I_PlayMusic();
}