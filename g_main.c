#include "g_main.h"
#include "g_mainmenu.h"
#include "g_fonts.h"
#include "g_textures.h"
#include "r_raycaster.h"
#include "n3d_re_trig.h"
#include "n3d_re_timing.h"
#include <stdlib.h>

uint16_t R_PlayerFace;


void G_Init()
{
    gameinfo.gamestate = GAMESTATE_STARTSCREEN;

    /*
     * Exact movement/render direction tables are extracted from the checked
     * original EXE by tools/extract_nite3w_trig_q10.py. Never fall back to
     * host libm: that would reintroduce non-original rounding.
     */
    const char* trig_path = getenv("N3D_TRIG_Q10");
    if(!trig_path || !*trig_path)
        trig_path = "N3D_TRIG_Q10.BIN";

    if(!N3D_RE_LoadTrigQ10(trig_path))
    {
        printf(
            "ERROR: exact Q10 trig table not loaded from %s\n"
            "Generate it with tools/extract_nite3w_trig_q10.py "
            "<NITE3W.EXE> N3D_TRIG_Q10.BIN\n",
            trig_path);
    }
    else
    {
        printf("loaded exact Q10 trig table from %s\n", trig_path);
    }

    /*
     * The original calibration clamps its measured mean to at least 40 ms.
     * Until the startup 5-sample render/present calibration is wired into this
     * old SDL shell, use that exact minimum baseline rather than arbitrary
     * float speed constants.
     */
    N3D_RE_SetTimingFromRawMean(40);
    printf(
        "gameplay timing baseline: move=%u turn=%u projectile=%u\n",
        n3d_timing.movement_substeps,
        n3d_timing.turn_degrees,
        n3d_timing.projectile_substeps);

    D_LoadDats();
    R_Init();
    I_InitMusic();
    R_LoadPCXFiles(renderer);
    R_SetPCX(PCX_HUD);
    G_LoadFonts();
    G_InitStartScreen();

    I_PlayMusic();
    // R_DumpSprites();

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

void G_DrawCurrentWeapon()
{
    R_DrawSprite(UI_WEAPON_X, UI_WEAPON_Y, sprites[UI_PLASMAGUN]);
}

void G_DrawPlayerFace()
{
    R_PlayerFace = UI_PLAYERFACE + p_player->health / 10;
    R_DrawSprite(UI_PLAYERFACE_X, UI_PLAYERFACE_Y, sprites[R_PlayerFace]);
}

void G_UpdateGame()
{
    I_HandleKeyboard();
    switch (gameinfo.gamestate)
    {
    case GAMESTATE_STARTSCREEN:
        G_UpdateStartScreen();
        overlay = PCX_START_SCREEN;
        break;
    case GAMESTATE_PLAYING:
        G_UpdateMainGame();
        overlay = PCX_HUD;
        break;
    case GAMESTATE_MAINMENU:
        overlay = PCX_MAIN_MENU;
        G_UpdateMainMenu();
        if (G_MainMenuScreenDone())
        {
            gameinfo.gamestate = GAMESTATE_PLAYING;
        }
        break;
    case GAMESTATE_PLEASE_WAIT:
        break;
    default:
        break;
    }
}