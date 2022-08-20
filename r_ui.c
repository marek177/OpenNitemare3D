#include "r_ui.h"

byte uiframebuffer[320*200];
byte uitexturebuffer[320*200*4];
SDL_Texture* uitexture;

pcx overlay;
SDL_Texture* pcx_images[14];

extern SDL_Renderer* renderer;

//TODO: remake N3D bitmap font 
void R_DrawText(const char* text, int x, int y, Font font, int color)
{
    SDL_Color fg;
    fg.a = 255;
    R_GetColor(color, &fg.r,&fg.g,&fg.b);

    SDL_Surface* surface = TTF_RenderText_Solid(fonts[font], text, fg);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    SDL_Rect rect = 
    {
        .x = x,
        .y = y
    };

    SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);

}

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

        uitexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 320, 200);
        SDL_SetTextureBlendMode(uitexture, SDL_BLENDMODE_BLEND);
    }


    D_FreeDat(&UIF);
}

void R_SetPCX(pcx newoverlay)
{
    overlay = newoverlay;
}

void R_FreePCX(pcx tobefreed)
{
    SDL_DestroyTexture(pcx_images[overlay]);
}

void R_InitUI()
{
}

void R_DrawUI(SDL_Renderer* renderer)
{
    R_DrawUISprites(renderer);
    
    // for(int x = 0; x < 320; x++)
    // {
    //     for(int y = 0; y < 200; y++)
    //     {
    //         byte i = uiframebuffer[(x+y)+320];
    //         byte r, g, b, a;
    //         r = palette[i * 3];
    //         g = palette[(i * 3) + 1];
    //         b = palette[(i * 3) + 2];

    //         a = (i == 0) ? 0 : 255;

    //         uitexturebuffer[4 * (x + y * 320)] = r;
    //         uitexturebuffer[4 * (x + y * 320) + 1] = g;
    //         uitexturebuffer[4 * (x + y * 320) + 2] = b;
    //         uitexturebuffer[4 * (x + y * 320) + 3] = a;

    //     }
    // }
    SDL_Rect dst;
    dst.w = 320;
    dst.h = 240;
    dst.x = 0;
    dst.y = 0;
    // SDL_UpdateTexture()
    SDL_UpdateTexture(uitexture, NULL, uitexturebuffer, 320*4);
    SDL_RenderCopy(renderer, uitexture, NULL, &dst);
}

void R_ClearUI()
{
}

void R_DrawSprite(int x, int y, r_sprite sprite)
{
    for(int tx = 0; tx < sprite.width; tx++)
    {
        for(int ty = 0; ty < sprite.height; ty++)
        {
            int i = ((tx+x)+((ty+y)*320)) * 4;
            int j = sprite.data[tx + ty * sprite.width] * 3;
            uitexturebuffer[i]  = palette[j];
            uitexturebuffer[i+1]  = palette[j+1];
            uitexturebuffer[i+2]  = palette[j+2];
            uitexturebuffer[i+3]  = 255;
        }
    }
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