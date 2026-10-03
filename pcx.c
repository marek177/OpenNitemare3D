#include "pcx.h"
#include "typedefs.h"

typedef struct pcxImage
{

}pcxImage;


//we only need like one variable from the header lol
typedef struct pcxHeader
{
    uint16_t colorPlane;
}pcxHeader;

void PCX_ReadImages()
{
    for(int i = 0; i < PCX_ORDER_NOW; i++)
    {
        void* output = malloc(320*200);

        int x = 0;
        int y = 0;
    }
}

void* PCX_GetImage(pcx id)
{

}