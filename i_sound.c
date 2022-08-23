#include "i_sound.h"
#include <SDL2/SDL.h>
#include "i_ibk.h"
Mix_Music* current_midi;

//my favorite part of having my own Doom port is I can just nab the sound code I wrote for it and only have to rewrite a little bit of it :)
//edit from like two hours after I wrote the above line, it was more than a little bit ;w;

#define SAMPLECOUNT		512
#define SFX_SAMPLERATE 11025	// Hz
#define MUS_SAMPLERATE 44100

#define SFMIDI_LOADERFRAMES 2048
#define MIDI_BUFFERSIZE SFMIDI_LOADERFRAMES * 2

static unsigned char midiBuffer[1024*1024];
int midiLength;

//stores rendered MIDI output
Uint16* midiPCMData = NULL;


//fluidsynth stuff
fluid_settings_t *settings;
fluid_synth_t *synth;
fluid_audio_driver_t *adriver;
fluid_player_t *player;


SDL_AudioStream* midiStream;
SDL_AudioDeviceID audioDevice;

//delete and recreate the player because Fluidsynth is awful
void ResetFluidsynthPlayer();

int _fontID = 0;
fluid_sfont_t* I_GetSoundFont()
{
    return fluid_synth_get_sfont_by_id(synth, _fontID);
}

fluid_synth_t* I_GetFluidSynth()
{
    return synth;
}

void I_InitMusic()
{
    SDL_Init(SDL_INIT_AUDIO);
    I_SetVolume(10);

    settings = new_fluid_settings();

    synth = new_fluid_synth(settings);
    fluid_synth_set_sample_rate(synth, MUS_SAMPLERATE);
    

    fluid_settings_setstr(settings, "audio.driver", "alsa");
    // fluid_settings_setint(settings, "audio.period-size", 64);
    midiPCMData = malloc(MIDI_BUFFERSIZE);

    _fontID = fluid_synth_sfload(synth, "chorium.sf2", 1);

    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    SDL_zero(obtained);
    desired.freq = MUS_SAMPLERATE;
    desired.format = AUDIO_S16;
    desired.channels = 2;
    desired.silence = 0;
    desired.samples = MIDI_BUFFERSIZE;

    audioDevice = SDL_OpenAudioDevice(NULL, 0, &desired, NULL, 0);
    midiStream = SDL_NewAudioStream(AUDIO_S16, 2, MUS_SAMPLERATE, AUDIO_S16, 2, MUS_SAMPLERATE);

    if (!midiStream) 
        printf("failed to create stream: %s\n", SDL_GetError());

    SDL_PauseAudioDevice(audioDevice, 0);

    I_LoadIBK();
}

void I_SetVolume(byte volume)
{
    Mix_VolumeMusic(volume);
}

void I_UpdateSound()
{
    
    int doneplaying = fluid_player_get_status(player) == FLUID_PLAYER_DONE;

    if(doneplaying)
        ResetFluidsynthPlayer();

    if(fluid_synth_write_s16(synth, SFMIDI_LOADERFRAMES / 2, midiPCMData, 0, 2, midiPCMData, 1, 2) == FLUID_FAILED)
        printf("failed to write fluid synth :3\n");
    
    int rc = SDL_AudioStreamPut(midiStream, midiPCMData, SFMIDI_LOADERFRAMES * sizeof (Sint16));
    if (rc == -1) {
        printf("Uhoh, failed to put samples in stream: %s\n", SDL_GetError());
        return;
    }

    SDL_AudioStreamFlush(midiStream);

    static float converted[SFMIDI_LOADERFRAMES];
    // this is in bytes, not samples!
    int gotten = SDL_AudioStreamGet(midiStream, converted, sizeof (converted));
    if (gotten == -1) {
        printf("Uhoh, failed to get converted data: %s\n", SDL_GetError());
    }else
    {
        if(SDL_QueueAudio(audioDevice, converted, gotten) == -1)
            printf("failed to queue audio: %s\n", SDL_GetError());
    }
}


void I_PauseMusic()
{
    Mix_PauseMusic();
}

void I_PlayMusic()
{
    ResetFluidsynthPlayer();
    // Mix_PlayMusic(current_midi, -1);
}

void I_ChangeSong(int id)
{
    dat_entry_t entry = SND.entries[id+1];
    memcpy(midiBuffer, D_GetData(entry), entry.length);
    midiLength = entry.length;
    SDL_AudioStreamFlush(midiStream);
    SDL_ClearQueuedAudio(audioDevice);
    I_PlayMusic();
}


//delete and recreate the player because Fluidsynth is awful
void ResetFluidsynthPlayer()
{
  if(player)
    delete_fluid_player(player);

  player = new_fluid_player(synth);

  fluid_player_add_mem(player, midiBuffer, midiLength);
  fluid_player_play(player);
}