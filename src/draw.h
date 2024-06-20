#ifndef DRAW_HG__
#define DRAW_HG__
//#include <SDL2/SDL.h>
#include "indirect_sdl.h"

int SDL_RenderDrawCircle(struct sdl_syms *sdl, SDL_Renderer *renderer, int x, int y, int radius);
int SDL_RenderFillCircle(struct sdl_syms *sdl, SDL_Renderer *renderer, int x, int y, int radius);

#endif // DRAW_HG__
