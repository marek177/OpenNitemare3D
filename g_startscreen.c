#include "g_startscreen.h"

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
    startscreen = malloc(sizeof(startscreen));
}
