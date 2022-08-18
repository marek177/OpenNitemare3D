#include "i_keyboard.h"
Uint8* keys;

void I_HandleKeyboard()
{
    keys = SDL_GetKeyboardState(NULL);
}

bool I_IsKeyDown(SDL_Scancode key)
{
    return keys[key];
}