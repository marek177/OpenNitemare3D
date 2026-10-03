#include "i_time.h"
#include <SDL2/SDL.h>

uint64_t previous = 0;
uint64_t delta = 0;

int I_FrameTime()
{
    return (int)delta;
}

void I_UpdateTime()
{
    const uint64_t current = SDL_GetTicks();

    if(previous == 0)
    {
        previous = current;
        delta = 0;
        return;
    }

    delta = current - previous;
    previous = current;
}
