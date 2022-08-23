#include "g_mainmenu.h"
#include "i_sound.h"
#include "r_ui.h"
#include "i_time.h"
#include "g_fonts.h"
#include "g_game.h"
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
    "Quit"
};

void (*_menuItemUseFunctions[])() =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

int _menuItemCount = 6;


bool G_MainMenuScreenDone()
{
    return _g_mainMenuComplete;
}

void G_InitMainMenu()
{
    _g_mainMenuComplete = false;
    I_ChangeSong(MIDI_FANTASIA);   
}

void G_UpdateMainMenu()
{
    static uint64_t timer = 0;
    timer += I_FrameTime();

    if(I_IsKeyDown(SDL_SCANCODE_RETURN) && timer > 100)
    {
        timer = 0;
        gameinfo.gamestate = GAMESTATE_PLAYING;
        _g_mainMenuComplete = true;
        G_StartMainGame();
    }

    for(int i = 0; i < _menuItemCount; i++)
    {
        // R_DrawText(_menuItemNames[i], _menuTextX, _menuTextY + y, FONT_MAINMENU, 170);
        R_DrawText(_menuTextX, _menuTextY + i * 8, 244, _menuItemNames[i] );
    }
}