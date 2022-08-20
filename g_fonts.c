#include "g_fonts.h"

TTF_Font* fonts[] = {
    NULL,
    NULL
};

void G_LoadFonts()
{
    TTF_Init();
    fonts[FONT_MAINMENU] = TTF_OpenFont("FFFFORWA.TTF", 8);
}
