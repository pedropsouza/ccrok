#include "SDL_system.h"
#include "SDL_video.h"
#include "ivec.h"
#include "volksrace.h"
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION

const ivec ref_window_dims = {.x = 850, .y = 478};

void
graphics_init(volks *s) {
  volks_rdr *rdat = &s->rdr;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  rdat->win = SDL_CreateWindow("volks", SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED, s->sdata.window_w, s->sdata.window_h, SDL_WINDOW_RESIZABLE);
  rdat->rdr = SDL_CreateRenderer(rdat->win, s->sdata.rdr_backend, SDL_RENDERER_ACCELERATED);
  if (rdat->rdr == NULL) {
    fprintf(stderr, "failed to create SDL renderer: %s\n", SDL_GetError());
    return; // FIXME: add proper error handling up the callstack
  }

  SDL_RendererInfo rdr_info;
  printf("available renderers:\n");
  for (int i = 0; i < SDL_GetNumRenderDrivers(); i++) {
    SDL_GetRenderDriverInfo(i, &rdr_info);
    printf("\t%d: %s\n", i, rdr_info.name);
  }
  SDL_GetRendererInfo(rdat->rdr, &rdr_info);
  printf("renderer: %s\n", rdr_info.name);

  SDL_SetRenderDrawBlendMode(rdat->rdr, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(rdat->rdr, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(rdat->rdr);

  // modified dimensions maybe
  SDL_GetRendererOutputSize(rdat->rdr, (int*)&s->sdata.window_w, (int*)&s->sdata.window_h);

  int smaller = (s->sdata.window_w > s->sdata.window_h) ?
    s->sdata.window_h : s->sdata.window_w;

  graphics_resize(s, (ivec){s->sdata.window_w,s->sdata.window_h});
}

void
graphics_resize(volks *s, ivec new_window_dims) {
  volks_rdr *rd = &s->rdr;
  rd->win_dims = new_window_dims;
  ivec nwd = rd->win_dims;
}

void
graphics_finish(volks *s) {
  volks_rdr *rdat = &s->rdr;
  SDL_DestroyRenderer(rdat->rdr);
  SDL_DestroyWindow(rdat->win);
  SDL_Quit();
}

void draw_fvec(volks *v, fvec origin, fvec vec) {
  volks_rdr *rd = &v->rdr;
  SDL_RenderDrawLine(rd->rdr, origin.x, origin.y, vec.x, vec.y);  
}
