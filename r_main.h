#ifndef R_RENDERER
#define R_RENDERER
#include "r_img.h"
#include "r_ui.h"
#include <SDL2/SDL_image.h>
#define RAYCAST_WIDTH 320
#define RAYCAST_HEIGHT 200

#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240
#define WINDOW_NAME "Nitemare 3D"

extern byte framebuffer[RAYCAST_WIDTH][RAYCAST_HEIGHT];
extern bool R_isopen;
extern r_sprite* sprites;
extern SDL_Window* window;
extern SDL_Renderer* renderer;
extern SDL_Event event;
// uint tickcount;

void R_SaveTexture(const char* file_name, SDL_Renderer* renderer, SDL_Texture* texture);
void R_DumpSprites();
void R_Init();
void R_Clear();
void R_DrawFrameBuffer();
void R_Present();
bool R_IsOpen();
void R_ProcessSDLWindowEvent();
void R_ProcessSDLInput();
void R_Close();

#endif
