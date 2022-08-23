#include "i_time.h"
#include <SDL2/SDL.h>
uint64_t previous = 0;
uint64_t delta = 0;
int I_FrameTime()
{
    return delta;
}

void I_UpdateTime()
{
    uint64_t current = SDL_GetTicks();
    delta = current - previous;
    previous = current;
}