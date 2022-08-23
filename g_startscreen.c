#include "g_startscreen.h"
#include "i_sound.h"
#include "g_game.h"
#include "g_mainmenu.h"

g_startscreen* startscreen;


void G_UpdateStartScreen()
{
    if(I_IsKeyDown(SDL_SCANCODE_RETURN))
    {
        gameinfo.gamestate = GAMESTATE_MAINMENU;
        G_InitMainMenu();
    }
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
