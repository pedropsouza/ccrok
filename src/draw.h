#ifndef DRAW_HG__
#define DRAW_HG__
#include <SDL2/SDL.h>

int SDL_RenderDrawCircle(SDL_Renderer *renderer, int x, int y, int radius);
int SDL_RenderFillCircle(SDL_Renderer *renderer, int x, int y, int radius);

#endif // DRAW_HG__
