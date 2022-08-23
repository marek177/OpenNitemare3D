#ifndef I_SOUND
#define I_SOUND
#include "d_dat.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <fluidsynth.h>


//some of the midis had names hidden in their metadata,
//others I just assigned names myself
typedef enum midi
{
    MIDI_HAUNTEDHOUSE_THEME = 0,
    MIDI_STAR = 3,
    MIDI_FANTASIA = 11,
    MIDI_E1M1 = 10
}midi;

extern Mix_Music* current_midi;
void I_UpdateSound();
void I_InitMusic();
fluid_sfont_t* I_GetSoundFont();
fluid_synth_t* I_GetFluidSynth();
void I_PauseMusic();
void I_PlayMusic();
void I_ChangeSong(int id);
void I_SetVolume(byte volume);

#endif