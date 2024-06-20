#include "indirect_sdl.h"

void *try_find_sdl2_lib(void) {
	char *candidates[] = {
		"libSDL2-2.0.so",
		"libSDL2-2.0.0.dylib",
		"SDL2.dll"
	};
	void *lib = NULL;
	for (size_t i = 0; i < (sizeof(candidates) / sizeof(*candidates)); ++i) {
		if ((lib = cosmo_dlopen(candidates[i], RTLD_LAZY))) return lib;
	}

	printf("Couldn't find SDL library (%s), tried the following names: ", cosmo_dlerror());
	for (size_t i = 0; i < (sizeof(candidates) / sizeof(*candidates)); ++i) {
		printf("\"%s\" ", candidates[i]);
	}
	printf("\n");

	return NULL;
}

struct sdl_syms *try_get_sdl2_syms(void) {
	void *sdl2 = try_find_sdl2_lib();
	if (!sdl2) return NULL;
	struct sdl_syms *syms = calloc(1, sizeof(*syms));
	*syms = (struct sdl_syms){
		.lib = sdl2,
		.SDL_VideoInit              = cosmo_dlsym(sdl2, "SDL_VideoInit"),
		.SDL_VideoQuit              = cosmo_dlsym(sdl2, "SDL_VideoQuit"),
		.SDL_Quit                   = cosmo_dlsym(sdl2, "SDL_Quit"),
		.SDL_Delay                  = cosmo_dlsym(sdl2, "SDL_Delay"),
		.SDL_GetError               = cosmo_dlsym(sdl2, "SDL_GetError"),
		.SDL_SetWindowIcon          = cosmo_dlsym(sdl2, "SDL_SetWindowIcon"),
		.SDL_FreeSurface            = cosmo_dlsym(sdl2, "SDL_FreeSurface"),
		.SDL_CreateRGBSurfaceFrom   = cosmo_dlsym(sdl2, "SDL_CreateRGBSurfaceFrom"),
		.SDL_CreateWindow           = cosmo_dlsym(sdl2, "SDL_CreateWindow"),
		.SDL_CreateRenderer         = cosmo_dlsym(sdl2, "SDL_CreateRenderer"),
		.SDL_CreateTexture          = cosmo_dlsym(sdl2, "SDL_CreateTexture"),
		.SDL_DestroyTexture         = cosmo_dlsym(sdl2, "SDL_DestroyTexture"),
		.SDL_DestroyRenderer        = cosmo_dlsym(sdl2, "SDL_DestroyRenderer"),
		.SDL_DestroyWindow          = cosmo_dlsym(sdl2, "SDL_DestroyWindow"),
		.SDL_RenderPresent          = cosmo_dlsym(sdl2, "SDL_RenderPresent"),
		.SDL_RenderSetLogicalSize   = cosmo_dlsym(sdl2, "SDL_RenderSetLogicalSize"),
		.SDL_SetRenderDrawBlendMode = cosmo_dlsym(sdl2, "SDL_SetRenderDrawBlendMode"),
		.SDL_SetRenderDrawColor     = cosmo_dlsym(sdl2, "SDL_SetRenderDrawColor"),
		.SDL_SetTextureBlendMode    = cosmo_dlsym(sdl2, "SDL_SetTextureBlendMode"),
		.SDL_RenderSetScale         = cosmo_dlsym(sdl2, "SDL_RenderSetScale"),
		.SDL_RenderClear            = cosmo_dlsym(sdl2, "SDL_RenderClear"),
		.SDL_PollEvent              = cosmo_dlsym(sdl2, "SDL_PollEvent"),
		.SDL_UpdateTexture          = cosmo_dlsym(sdl2, "SDL_UpdateTexture"),
		.SDL_RenderCopy             = cosmo_dlsym(sdl2, "SDL_RenderCopy"),
		.SDL_UpdateWindowSurface    = cosmo_dlsym(sdl2, "SDL_UpdateWindowSurface"),
		.SDL_PointInRect            = cosmo_dlsym(sdl2, "SDL_PointInRect"),
	};
	// Quick check to make sure they were all found
	for (size_t i = 0; i < (sizeof(struct sdl_syms) / sizeof(void *)); ++i) {
		if (!((void **)syms)[i]) {
			printf("sdl_syms[%zu] is NULL, check for typos\n", i);
			free(syms);
			return NULL;
		}
	}
	return syms;
}
