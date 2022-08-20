#ifndef G_FONTS
#define G_FONTS
#include <SDL2/SDL_ttf.h>

typedef enum Font
{
    FONT_MAINMENU,
    FONT_UISMALL
}Font;

extern TTF_Font* fonts[];

void G_LoadFonts();

#endif