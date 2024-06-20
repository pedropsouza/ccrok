#include "SDL_video.h"
#include "ivec.h"
#include "solr.h"
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const ivec ref_window_dims = {.x = 850, .y = 478};
//const ivec card_dims = {.x = 88, .y = 124};
const ivec card_dims = {.x = 44, .y = 60};

#define cw card_dims.x
#define ch card_dims.y
#define hpad 12

ivec ref_stack_areas[STACK_BOARD_MAX] = {
  [0]  = (ivec){.x = hpad,                   .y = ch/2,     },
  [1]  = (ivec){.x = hpad + 1 * (hpad + cw), .y = ch/2      },
  [2]  = (ivec){.x = hpad + 2 * (hpad + cw), .y = ch/2      },
  [3]  = (ivec){.x=2*hpad + 3 * (hpad + cw), .y = ch/2      },
  [4]  = (ivec){.x=2*hpad + 4 * (hpad + cw), .y = ch/2},
  [5]  = (ivec){.x=2*hpad + 5 * (hpad + cw), .y = ch/2},
  [6]  = (ivec){.x=2*hpad + 6 * (hpad + cw), .y = ch/2},
  [7]  = (ivec){.x = hpad + 0 * (hpad + cw), .y = 140+ch/2,     },
  [8]  = (ivec){.x = hpad + 1 * (hpad + cw), .y = 140+ch/2,     },
  [9]  = (ivec){.x = hpad + 2 * (hpad + cw), .y = 140+ch/2,     },
  [10]  = (ivec){.x = hpad + 3 * (hpad + cw), .y = 140+ch/2,     },
  [11]  = (ivec){.x = hpad + 4 * (hpad + cw), .y = 140+ch/2,     },
  [12]  = (ivec){.x = hpad + 5 * (hpad + cw), .y = 140+ch/2,     },
  [13]  = (ivec){.x = hpad + 6 * (hpad + cw), .y = 140+ch/2,     },
  [14]  = (ivec){.x = hpad + 7 * (hpad + cw), .y = 140+ch/2,     },
};

#undef cw
#undef ch

static SDL_Texture *
tex_from_file(SDL_Renderer *rdr, const char* filename) {
  FILE *f = fopen(filename, "r");
  int w,h,chnum;
  stbi_uc *pix = stbi_load_from_file(f, &w, &h, &chnum, 4);
  for (int i = 0; i < w*h; i++) {
    stbi_uc *head = pix + i * 4;
    stbi_uc *r = head, *g = head + 1, *b = head + 2, *a = head + 3;
    if (*g == 128 && *b == 128) { *a = 0; }
  }
  printf("%s, %dx%d, chn %d\n", filename, w, h, chnum);
  SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(pix, w, h, 32, 4*w, 0x0000ff, 0x00ff00, 0xff0000, 0xff000000);
  SDL_Texture *tex = SDL_CreateTextureFromSurface(rdr, surf);
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
  SDL_FreeSurface(surf);
  stbi_image_free(pix);
  fclose(f);
  return tex;
}

void
graphics_init(solr *s) {
  solr_rdr *rdat = &s->rdr;
  memset(&rdat->stack_areas, 0, sizeof(rdat->stack_areas));

  SDL_Init(SDL_INIT_VIDEO);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  rdat->win = SDL_CreateWindow("solr", SDL_WINDOWPOS_UNDEFINED,
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

  rdat->backs         = tex_from_file(rdat->rdr, "resources/cards/deck_mahjong_backs.png");
  rdat->locked_icon   = tex_from_file(rdat->rdr, "resources/cards/Card_locked_icon.png");
  rdat->suits = tex_from_file(rdat->rdr, "resources/cards/deck_mahjong_light_1.png");

  int cw = card_dims.x, ch = card_dims.y;
  
  graphics_resize(s, (ivec){s->sdata.window_w,s->sdata.window_h});
}

void
graphics_resize(solr *s, ivec new_window_dims) {
  solr_rdr *rd = &s->rdr;
  rd->win_dims = new_window_dims;
  ivec nwd = rd->win_dims;
  rd->card_dims = ivec_div(ivec_mul(card_dims, nwd), ref_window_dims);
  printf("new card dims: %ld,%ld\n", rd->card_dims.x, rd->card_dims.y);
  for (int i = 0; i < STACK_MAX; i++) {
    ivec area;
    if (i < STACK_BOARD_MAX) {
      area = ivec_div(ivec_mul(ref_stack_areas[i], nwd), ref_window_dims);
    } else {
      area = (ivec){0,0};
    }
    rd->stack_areas[i].x = area.x;
    rd->stack_areas[i].y = area.y;
    rd->stack_areas[i].w = rd->card_dims.x;
    rd->stack_areas[i].h = rd->card_dims.y;
  }
  for (int i = 0; i < STACK_MAX; i++) {
    update_stack_dims(s, stack_from_id(s, i));
  }
}

void
graphics_finish(solr *s) {
  solr_rdr *rdat = &s->rdr;
  SDL_DestroyTexture(rdat->backs);
  SDL_DestroyTexture(rdat->suits);
  SDL_DestroyTexture(rdat->locked_icon);
  SDL_DestroyRenderer(rdat->rdr);
  SDL_DestroyWindow(rdat->win);
  SDL_Quit();
}
