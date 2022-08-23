#include "g_main.h"
#include "r_raycaster.h"
#include "i_time.h"
int main()
{
    G_Init();
    while(R_IsOpen())
    {
        I_UpdateTime();
        R_Clear();
        R_ProcessSDLInput();
        R_DrawFrameBuffer();
        R_DrawUI(renderer);
        G_UpdateGame();
        R_Present();
        I_UpdateSound();
    }
    // R_Close();
}