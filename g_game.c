#include "g_game.h"
#include "g_textures.h"
#include "m_obj.h"
#include "n3d_re_runtime.h"
#include "g_think.h"
#include "n3d_re_player.h"
#include "n3d_re_definitions.h"


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
    G_HandleRecoveredGuardThinking();
    G_ShowWalls();

    R_DrawRaycaster();
}

void G_LoadEpisode(uint8_t episode)
{
    printf("loading episode %d\n", episode);
    gameinfo.episode = episode;

    if(N3D_RE_LoadEpisodeDefinitions(episode))
    {
        printf("loaded definitions: %u WALLS, %u OBJECTS\n",
               n3d_wall_definitions.count,
               n3d_object_definitions.count);
    }
    else
    {
        printf("definition files WALLS.%u / OBJECTS.%u not fully available\n",
               episode, episode);
    }
}


//dear David P Gray, why?
void G_CreateMapObject(byte id, uint8_t x, uint8_t y)
{
    if(id > 0 && id <= MT_StartpositionW)
    {
        N3D_RE_InitPlayerAtTile(x, y);

        if(!p_player)
        {
            obj_t* spawned_player = M_Spawn(id, x, y, id-1);
            P_InitPlayer(spawned_player);
        }

        p_player->health = n3d_player.health;
        p_player->x =
            (float)n3d_player.world_x / (float)N3D_WORLD_UNITS_PER_TILE;
        p_player->y =
            (float)n3d_player.world_y / (float)N3D_WORLD_UNITS_PER_TILE;
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
        SDL_Log("spawned Skeleton at {%d,%d}\n",x,y);
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

    byte* data = malloc(N3D_MAP_LEVEL_BYTES);
    char filename[64];

    sprintf(filename, "MAP.%d", gameinfo.episode);
    FILE* file = fopen(filename, "rb");
    if(!file)
    {
        printf("failed to open %s\n", filename);
        free(data);
        return;
    }

    const long level_offset =
        N3D_MAP_HEADER_BYTES + ((long)level * N3D_MAP_LEVEL_BYTES);
    fseek(file, level_offset, SEEK_SET);

    if(fread(data, N3D_MAP_LEVEL_BYTES, 1, file) != 1)
    {
        printf("failed to read level payload from %s\n", filename);
        fclose(file);
        free(data);
        return;
    }

    if(!N3D_RE_LoadMapPayload(data, N3D_MAP_LEVEL_BYTES))
    {
        printf("failed to decode MAP payload\n");
        fclose(file);
        free(data);
        return;
    }

    for(int cell = 0; cell < N3D_MAP_WIDTH * N3D_MAP_HEIGHT; cell++)
    {
        uint8_t x = (uint8_t)(cell % N3D_MAP_WIDTH);
        uint8_t y = (uint8_t)(cell / N3D_MAP_WIDTH);
        byte object_id = data[cell * N3D_MAP_CELL_BYTES + 1];
        G_CreateMapObject(object_id, x, y);
    }

    if(gameinfo.mapdata)
        free(gameinfo.mapdata);

    gameinfo.mapdata = malloc(N3D_MAP_WIDTH * N3D_MAP_HEIGHT);
    for(int cell = 0; cell < N3D_MAP_WIDTH * N3D_MAP_HEIGHT; cell++)
        gameinfo.mapdata[cell] = data[cell * N3D_MAP_CELL_BYTES];

    fclose(file);
    free(data);

    printf("recovered runtime: %u OBJECT, %u GUARD\n",
           n3d_object_count, n3d_guard_count);
}
