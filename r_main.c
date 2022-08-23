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

    // typedef struct header
    // {
    //     byte w,h;
    //     byte unknown[7];
    // }header;

    // for(int k = 0; k < 3; k++)
    // {

        byte* data = D_GetData(UIF.entries[2]);

        int16_t offset = 2;

        // Setup the font array (127 characters)


        for (int i = 1; i < 128; i++) {
            byte height = *(data + offset);
            byte width = 8;//*(data + offset + 1);

            printf("character {w%d,h%d}\n", width, height);

            int16_t size = height * ((width+7) >> 3);
            

            byte* buffer = malloc((width*height)*4);

            for(int j = 0; j < size; j++)
                reverseByte(&data[offset + 2 + j]);


            for(int y = 0; y < height; ++y)
            {
                for(int x = 0; x < width; ++x)
                {
                    int pixel = y * width + x;
                    int bitpos = pixel % 8;
                    int offset_ = pixel / 8;
                    byte bitTest = (1 << bitpos);

                    byte c = 10;
                                 
                    if((data[offset + 2 + offset_] & bitTest) == bitTest)
                    {
                        c = 213;
                    }

                    byte r,g,b;
                    R_GetColor(c, &r,&g, &b);       

                    int off = pixel * 4;
                    buffer[off] = b;
                    buffer[off+1] = g;
                    buffer[off+2] = r;
                    buffer[off+3] = 255;
                }
            }

            char name[64];

            sprintf(name, "font/%c.png", i);
            printf("%s\n", name);

            SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(buffer, width, height, 32, width*4, 0, 0, 0, 0);
            IMG_SavePNG(surface, name);
            offset += 2 + size;
            
        

            free(buffer);
        }
        
    // }
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