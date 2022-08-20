#include "g_main.h"
#include "g_mainmenu.h"
#include "g_fonts.h"
#include "r_raycaster.h"

void G_Init()
{

    gameinfo.gamestate = GAMESTATE_STARTSCREEN;
    
    D_LoadDats();
    R_Init();
    I_InitMusic();
    R_LoadPCXFiles(renderer);
    R_SetPCX(PCX_HUD);
    G_LoadFonts();
    G_InitStartScreen();
    
    I_PlayMusic();
    //R_DumpSprites();

    gameinfo.version = ENGINEVERSION_V2_0;

    switch (gameinfo.version)
    {
    case ENGINEVERSION_V2_0:
        printf("Emulating engine version 2.0\n");
        break;
    
    default:
        break;
    }
}


void G_UpdateGame()
{
    I_HandleKeyboard();
    switch(gameinfo.gamestate)
    {
    case GAMESTATE_STARTSCREEN:
        G_UpdateStartScreen();
        overlay = PCX_START_SCREEN;
        if(G_StartScreenDone())
        {
            gameinfo.gamestate = GAMESTATE_MAINMENU;
            // G_StartMainGame();
            G_InitMainMenu();
        }
        break;
    case GAMESTATE_PLAYING:
        G_UpdateMainGame();
        overlay = PCX_HUD;
        break;
    case GAMESTATE_MAINMENU:
        overlay = PCX_MAIN_MENU;
        G_UpdateMainMenu();
        break;
    case GAMESTATE_PLEASE_WAIT:
        break;
    default:
        break;
    }
 }