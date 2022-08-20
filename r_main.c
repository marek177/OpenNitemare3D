#include "r_main.h"

byte framebuffer[RAYCAST_WIDTH * RAYCAST_HEIGHT * 3];
bool R_isopen;
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Event event;
SDL_Texture* frameTexture;

void R_Init()
{
    window = SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND)
;    R_isopen = true;
    R_LoadPalette();
    R_InitUI();

    frameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, RAYCAST_WIDTH, RAYCAST_HEIGHT);
}

void reverseByte(byte *data) {
	byte maskIn = 0x80;
	byte maskOut = 0x01;
	byte result = 0;

	for (byte i = 0; i < 8; i++, maskIn >>= 1, maskOut <<= 1) {
		if (*data & maskIn)
			result |= maskOut;
	}

	*data = result;
}

void DumpMysteryImages()
{
    for(int k = 0; k < 3; k++)
    {

        byte* data = D_GetData(UIF.entries[k]);

        int16_t offset = 257;                               // Start at fontdata[2] ([0],[1] used for height,width)

        // Setup the font array (127 characters)


        for (int i = 1; i < 128; i++) {
            // // _font[_fnt][i] = _fontdata[_fnt] + offset;
            // byte width = *(data+offset);
            // byte height  = *(data+offset+1);

            // int16_t size = height * ((width + 7) >> 3);
            int size = 7*6;
            
            byte* buffer = malloc((size)*4);
            for (int j = 0; j < size; j++)
            {
                reverseByte(&data[offset+2+j]);

                byte c = data[offset+2+j];
                byte r, g, b;
                R_GetColor(c, &r, &g, &b);
                
                int buffoff = j * 4;
                buffer[buffoff] = r;
                buffer[buffoff + 1] = g;
                buffer[buffoff + 2] = b;
                buffer[buffoff + 3] = 255;
            }

            char* filename[256];
            sprintf(filename, "font/dump_%d_%d.png", i, k);
                //R_SaveTexture(filename, renderer, texture);

            SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(buffer, 7, 6, 32, 7*4, 0, 0, 0, 0);
            IMG_SavePNG(surface, filename);
        }

        // int offset = 0;
        // int k = 0;
        // while(1)
        // {
        //     byte w = data[offset];
        //     byte h = data[offset+1];

        //     byte* buffer = malloc((w*h)*4);
        //     data++;

        //     printf("w:%d h:%d\n", w, h);

        //     for(int j = 0; j < w*h; j++)
        //     {
        //         byte pixel = data[j+2+offset];
        //         byte r, g, b;
        //         R_GetColor(pixel, &r, &g, &b);

        //         int buffoff = j * 4;
        //         buffer[buffoff] = r;
        //         buffer[buffoff + 1] = g;
        //         buffer[buffoff + 2] = b;
        //         buffer[buffoff + 3] = 255;
        //     }
        //     char* filename[256];
        //     sprintf(filename, "dump_%d_%d.png", i, k++);
        //     offset += (w*h) + 2;
        //     //R_SaveTexture(filename, renderer, texture);

        //     SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(buffer, w, h, 32, w*4, 0, 0, 0, 0);
        //     IMG_SavePNG(surface, filename);
        // }
        
        
    }
}

void R_DumpSprites()
{

    // byte* img = UIF.entries[0].data;
    DumpMysteryImages();

    

    for(int i = 0; i < imgCount-1127; i++)
    {
        byte width = sprites[i].width;
        byte height = sprites[i].height;
        byte* data = sprites[i].data;
        byte* buffer = malloc(width * height * 4);

        for(int j = 0; j < width*height; j++)
        {
            byte pixel = data[j];
            byte r, g, b;
            R_GetColor(pixel, &r, &g, &b);

            int buffoff = j * 4;
            buffer[buffoff] = b;
            buffer[buffoff + 1] = g;
            buffer[buffoff + 2] = r;
            buffer[buffoff + 3] = 255;
        }
       
        char* filename[256];
        sprintf(filename, "img/img_%d.png", i);
        //R_SaveTexture(filename, renderer, texture);

        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(buffer, width, height, 32, width*4, 0, 0, 0, 0);
        IMG_SavePNG(surface, filename);
    }
}

void R_DrawFrameBuffer()
{
    SDL_UpdateTexture(frameTexture, NULL, framebuffer, RAYCAST_WIDTH * 3);
    SDL_RenderCopy(renderer, frameTexture, NULL, NULL);
}

void R_Clear()
{
    R_ClearUI();
    SDL_RenderClear(renderer);
}

void R_Present()
{
    SDL_RenderPresent(renderer);
}

bool R_IsOpen()
{
    return R_isopen;
}

void R_ProcessSDLWindowEvent()
{
    switch (event.window.event)
    {
    case SDL_WINDOWEVENT_CLOSE:
        R_Close();
        break;
    
    default:
        break;
    }
}

void R_Close()
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    R_DumpSprites();
    R_isopen = false;
}

void R_ProcessSDLInput()
{
    while(SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_WINDOWEVENT:
            R_ProcessSDLWindowEvent();
            break;
        
        default:
            break;
        }
    }
    
}