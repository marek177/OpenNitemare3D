#include "g_startscreen.h"
#include "i_sound.h"

g_startscreen* startscreen;


void G_UpdateStartScreen()
{
}

bool G_StartScreenDone()
{
    return I_IsKeyDown(SDL_SCANCODE_RETURN);
}

void G_InitStartScreen()
{
    I_ChangeSong(MIDI_HAUNTEDHOUSE_THEME);
    startscreen = malloc(sizeof(startscreen));
}
