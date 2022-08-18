#ifndef I_KEYBOARD
#define I_KEYBOARD
#include "typedefs.h"
#include <SDL2/SDL.h>

void I_HandleKeyboard();
bool I_IsKeyDown(SDL_Scancode key);
#endif