#include "g_game.h"
#include "g_textures.h"
#include "m_obj.h"


bool G_GameIsDone()
{

}
void G_StartMainGame()
{
    G_LoadEpisode(1);
    R_LoadSprites(1);
    G_LoadLevel(0);
}

void G_UpdateMainGame()
{
    G_ShowWalls();

    R_DrawRaycaster();
}

void G_LoadEpisode(uint8_t episode)
{
    printf("loading episode %d\n", episode);
    gameinfo.episode = episode;
    
}


//dear David P Gray, why?
void G_CreateMapObject(byte id, uint8_t x, uint8_t y)
{
    if(id > 0 && id <= MT_StartpositionW)
    {
        if(!p_player)
        {
            p_player = M_Spawn(id, x, y, id-1);
            p_player->player = malloc(sizeof(player_t));
            p_player->health = 100;
        }

        p_player->x = x;
        p_player->y = y;
        return;
    }

    if(id >= MT_BatN && id <= MT_BatW)
    {
        SDL_Log("spawned Bat at {%d,%d}\n",x,y);
    }

    if(id >= MT_FrankensteinN && id <= MT_FrankensteinW)
    {
        SDL_Log("spawned Frankenstein at {%d,%d}\n",x,y);
    }

    if(id >= MT_MummyN && id <= MT_MummyW)
    {
        SDL_Log("spawned Mummy at {%d,%d}\n",x,y);
    }

    if(id >= MT_SkeletonN && id <= MT_SkeletonW)
    {
        SDL_Log("spawned Mummy at {%d,%d}\n",x,y);
    }
}

void G_ShowWalls()
{
    for(byte x = 0; x < 62; x++)
    {
        for(byte y = 0; y < 36; y++)
        {
            // uiframebuffer[256 + x][162 + y] = gameinfo.mapdata[x+y*64];
        }
    }
}

void G_LoadLevel(uint8_t level)
{
    I_ChangeSong(2);
    printf("loading level E%dL%d\n", gameinfo.episode, level + 1);
    byte* data = malloc(8192);
    unsigned char* filename[64];

    sprintf(filename, "MAP.%d", gameinfo.episode);
    FILE* file = fopen(filename, "rb");
    fseek(file, 514, SEEK_SET);
    long off = level * 8192;
    
    fread(data, 8192, 1, file);

    for(int i = 1; i < 8192; i+=2)
    {
        uint8_t x = i % 64;
        uint8_t y = i / 64;
        G_CreateMapObject(data[i], x,y);
    }

    gameinfo.mapdata = malloc(4096);
    
    for(int i = 0; i < 8192; i++)
    {
        if(i % 2 == 0)
        {
            gameinfo.mapdata[i/2] = data[i];
        }
    }

    
}