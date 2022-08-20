#include "g_mainmenu.h"
#include "i_sound.h"
#include "r_ui.h"
#include "g_fonts.h"

bool _g_mainMenuComplete = false;

int _menuTextX = 110;
int _menuTextY = 60;


/*
"New game",
            "Configure game...",
            "Load game...",
            "Instructions",
            "Demo",
            "Sound Editor",
            "Quit"
*/

const char* _menuItemNames[] = 
{
    "New game",
    "Configure game...",
    "Load game...",
    "Instructions",
    "Demo",
    "Sound Editor",
    "Quit"
};

void (*_menuItemUseFunctions[])() =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

int _menuItemCount = 7;


bool G_MainMenuScreenDone()
{
    return _g_mainMenuComplete;
}

void G_InitMainMenu()
{
    _g_mainMenuComplete = false;
    I_ChangeSong(MIDI_E1M1);
    
    
}

void G_UpdateMainMenu()
{
    if(I_IsKeyDown(SDL_SCANCODE_RETURN))
    {
        _g_mainMenuComplete = true;
    }

    for(int i = 0; i < _menuItemCount; i++)
    {
        int y = i * 12;
        R_DrawText(_menuItemNames[i], _menuTextX, _menuTextY + y, FONT_MAINMENU, 170);
    }
}