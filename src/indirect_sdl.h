#ifndef INDIRECT_SDL
#define INDIRECT_SDL

/* Adapted from https://github.com/vkoskiv/cosmo-sdl-template
 * Thanks a lot Valtteri
 */

#include <stdbool.h>

#define _COSMO_SOURCE
#include "libc/dlopen/dlfcn.h"

#include "SDL2/SDL_render.h"
#include "SDL2/SDL_events.h"

// TODO: Add all SDL functions
struct sdl_syms {
	void *lib;
	int (*SDL_VideoInit)(const char *driver_name);
	void (*SDL_VideoQuit)(void);
	void (*SDL_Quit)(void);
	void (*SDL_Delay)(uint32_t ms);
	const char *(*SDL_GetError)(void);
	void (*SDL_SetWindowIcon)(SDL_Window *window, SDL_Surface *icon);
	void (*SDL_FreeSurface)(SDL_Surface *surface);
	SDL_Surface *(*SDL_CreateRGBSurfaceFrom)(void *pixels, int width, int height, int depth, int pitch,
											uint32_t rmask, uint32_t gmask, uint32_t bmask, uint32_t amask);
	SDL_Window *(*SDL_CreateWindow)(const char *title, int x, int y, int w, int h, uint32_t flags);
	SDL_Renderer *(*SDL_CreateRenderer)(SDL_Window *window, int index, uint32_t flags);
	SDL_Texture *(*SDL_CreateTexture)(SDL_Renderer *renderer, uint32_t format, int access, int w, int h);
	void (*SDL_DestroyTexture)(SDL_Texture *texture);
	void (*SDL_DestroyRenderer)(SDL_Renderer *renderer);
	void (*SDL_DestroyWindow)(SDL_Window *window);
	void (*SDL_RenderPresent)(SDL_Renderer *renderer);
	int (*SDL_RenderSetLogicalSize)(SDL_Renderer *renderer, int w, int h);
	int (*SDL_SetRenderDrawBlendMode)(SDL_Renderer *renderer, SDL_BlendMode blend_mode);
	int (*SDL_SetRenderDrawColor)(SDL_Renderer *renderer, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	int (*SDL_SetTextureBlendMode)(SDL_Texture *texture, SDL_BlendMode blend_mode);
	int (*SDL_RenderSetScale)(SDL_Renderer *renderer, float scaleX, float scaleY);
	int (*SDL_RenderClear)(SDL_Renderer *renderer);
	int (*SDL_PollEvent)(SDL_Event *event);
	int (*SDL_UpdateTexture)(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch);
	int (*SDL_RenderCopy)(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect);
	int (*SDL_UpdateWindowSurface)(SDL_Window *window);
	int (*SDL_PointInRect)(const SDL_Point *p, const SDL_Rect *r);
};

void *try_find_sdl2_lib(void);
struct sdl_syms *try_get_sdl2_syms(void);

#endif // INDIRECT_SDL
