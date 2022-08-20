#include "r_raycaster.h"
#include "r_main.h"

void SortSprites()
{

}

void R_DrawRaycaster()
{
    for(int x = 0; x < 16; x++)
    {
        for(int y = 0; y < 16; y++)
        {
            int i = (x+(y*16)) * 3;

            framebuffer[i] = palette[i];
            framebuffer[i+1] = palette[i+1];
            framebuffer[i+2] = palette[i+2];
        }
    }
}