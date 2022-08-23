#include "r_ui.h"
#include "g_main.h"
#include "g_textures.h"
#include "g_inventory.h"
pcx overlay;
SDL_Texture* pcx_images[14];

extern SDL_Renderer* renderer;

//TODO: remake N3D bitmap font 
// void R_DrawText(const char* text, int x, int y, Font font, int color)
// {
//     SDL_Color fg;
//     fg.a = 255;
//     R_GetColor(color, &fg.r,&fg.g,&fg.b);

//     SDL_Surface* surface = TTF_RenderText_Solid(fonts[font], text, fg);
//     SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
//     SDL_FreeSurface(surface);

//     SDL_Rect rect = 
//     {
//         .x = x,
//         .y = y
//     };

//     SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);
//     SDL_RenderCopy(renderer, texture, NULL, &rect);
//     SDL_DestroyTexture(texture);

// }

void R_LoadPCXFiles(SDL_Renderer* renderer)
{
    for(int i = 0; i < 14; i++)
    {
        dat_entry_t entry = UIF.entries[i+3];
        SDL_RWops* data = SDL_RWFromMem(D_GetData(entry), entry.length);
        SDL_Surface* surface = IMG_LoadPCX_RW(data);

        uint32_t keyColor;
		keyColor = SDL_MapRGB(surface->format, 0, 0, 0);
		SDL_SetColorKey(surface, SDL_TRUE, keyColor);


        pcx_images[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_RWclose(data);
        SDL_FreeSurface(surface);
    }


    // D_FreeDat(&UIF);
}

void R_SetPCX(pcx newoverlay)
{
    overlay = newoverlay;
}

void R_FreePCX(pcx tobefreed)
{
    SDL_DestroyTexture(pcx_images[overlay]);
}

SDL_Texture* uiSpriteTexture;


void* fontData[3][128];

void R_LoadFonts()
{
    for(int k = 0; k < 3; k++)
    {
        byte* data = D_GetData(UIF.entries[k]);

        int16_t offset = 2;

        for (int i = 1; i < 128; i++)
        {
            byte height = *(data + offset);
            byte width = 8;//*(data + offset + 1);

            // printf("character {w%d,h%d}\n", width, height);

            int16_t size = height * ((width+7) >> 3);
            

            // byte* buffer = malloc((width*height)*4);

            for(int j = 0; j < size; j++)
                reverseByte(&data[offset + 2 + j]);

            fontData[k][i] = &data[offset];
            offset += 2 + size;
            
        }
    }
}

void R_InitUI()
{
    uiSpriteTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 64,64);
    R_LoadFonts();
}

int currentFont = 0;

void R_SetFont(int font)
{
    currentFont = font;
}

void PrintChar(int x, int y, uint8_t color, char* data)
{
    int height = data[0];
    int width = data[1];
    for(int sy = 0; sy < height; ++sy)
    {
        for(int sx = 0; sx < width; ++sx)
        {
            int pixel = sy * width + sx;
            int bitpos = pixel % width;
            int offset_ = pixel / width;
            byte bitTest = (1 << bitpos);
                                 
            if((data[2 + offset_] & bitTest) == bitTest)
            {
                byte r,g,b;
                R_GetColor(color, &r,&g, &b);       
                SDL_SetRenderDrawColor(renderer,r,g,b, 255);
                SDL_RenderDrawPoint(renderer, sx + x, sy+y);
            }
                    // int off = pixel * 4;
                    // buffer[off] = b;
                    // buffer[off+1] = g;
                    // buffer[off+2] = r;
                    // buffer[off+3] = 255;
            }
        }
}

void R_DrawText(int x, int y, uint8_t color, const char* str)
{
    void* font = fontData[currentFont];

    int xoff = 0;
    for(int i = 0; i < strlen(str) ; i++)
    {
        char* font = fontData[2][str[i]];
        PrintChar(x+xoff, y, color, font);
        xoff += font[0];
    }
}


void R_DrawUI(SDL_Renderer* renderer)
{
    R_DrawUISprites(renderer);

    if(gameinfo.gamestate == GAMESTATE_PLAYING)
    {
        G_DrawCurrentWeapon();
    }


    R_DrawPCX(renderer);


    if(gameinfo.gamestate == GAMESTATE_PLAYING)
    {
        G_DrawPlayerFace();
    
    
        //draw inventory items

        int keyX = 167; 
        int keyY = 162;

        for(int i = INVENTORYITEM_KEY_RED; i <= INVENTORYITEM_KEY_YELLOW; i++)
        {
            int keyOff = i - INVENTORYITEM_KEY_RED;
            
            if(i == INVENTORYITEM_KEY_BLUE)
            {
                keyY += 20;
                keyX = 167;
            }

            R_DrawSprite(keyX, keyY, sprites[UI_KEY_START + keyOff]);
            keyX += 22; //move to next key hole

            
        }
    
    }
}

void R_ClearUI()
{
}


void R_DrawSprite(int x, int y, r_sprite sprite)
{
    static byte textureBuffer[64*64*4];
    for(int tx = 0; tx < sprite.width; tx++)
    {
        for(int ty = 0; ty < sprite.height; ty++)
        {
            int i = ((tx)+((ty)*64)) * 4;
            int j = sprite.data[tx + ty * sprite.width];

            if(j != 41) //don't draw white pixels
            {
                byte r,g,b;
                R_GetColor(j, &r, &g, &b);
                textureBuffer[i]  = r;
                textureBuffer[i+1]  = g;
                textureBuffer[i+2]  = b;
                textureBuffer[i+3]  = 255;
            }
        }
    }
    SDL_UpdateTexture(uiSpriteTexture, NULL, textureBuffer, 64 * 4);
    
    SDL_Rect rect = 
    {
        .x = x,
        .y = y * 1.2,
        .w = sprite.width,
        .h = sprite.height * 1.2
    };

    SDL_Rect src = 
    {
        .x = 0,
        .y = 0,
        .w = sprite.width,
        .h = sprite.height
    };

    SDL_RenderCopy(renderer, uiSpriteTexture, &src, &rect);
}

void R_DrawUISprites(SDL_Renderer* renderer)
{
    
}

void R_DrawUIText(SDL_Renderer* renderer)
{

}

void R_DrawPCX(SDL_Renderer* renderer)
{
    SDL_RenderCopy(renderer, pcx_images[overlay], NULL, NULL);
}