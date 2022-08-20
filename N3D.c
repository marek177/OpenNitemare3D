#include "g_main.h"
#include "r_raycaster.h"
int main()
{
    G_Init();
    while(R_IsOpen())
    {
        R_Clear();
        R_ProcessSDLInput();
        R_DrawRaycaster();
        R_DrawFrameBuffer();
        R_DrawPCX(renderer);
        G_UpdateGame();
        R_DrawUI(renderer);
        R_Present();
        I_UpdateSound();
    }
    // R_Close();
}