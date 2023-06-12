#include "SDL_video.h"
#include "ivec.h"
#include "solr.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const ivec stack_areas[STACK_BOARD_MAX] = {
    [0]  = {.x = 12,                 .y = 64,     },
    [1]  = {.x = 12 + 1 * (12 + 88), .y = 140 + 64},
    [2]  = {.x = 12 + 2 * (12 + 88), .y = 140 + 64},
    [3]  = {.x = 12 + 3 * (12 + 88), .y = 140 + 64},
    [4]  = {.x = 12 + 4 * (12 + 88), .y = 140 + 64},
    [5]  = {.x = 12 + 5 * (12 + 88), .y = 140 + 64},
    [6]  = {.x = 12 + 6 * (12 + 88), .y = 140 + 64},
    [7]  = {.x = 12 + 7 * (12 + 88), .y = 140 + 64},
    [8]  = {.x = 12 + 4 * (12 + 88), .y = 64,     },
    [9]  = {.x = 12 + 5 * (12 + 88), .y = 64,     },
    [10] = {.x = 12 + 6 * (12 + 88), .y = 64,     },
    [11] = {.x = 12 + 7 * (12 + 88), .y = 64,     },
    [12] = {.x = 12 + 1 * (12 + 88), .y = 64,     },
};
const ivec ref_window_dims = {.x = 850, .y = 478};
const ivec card_dims = {.x = 88, .y = 124};
ivec card_dims_scaled;
ivec stack_areas_scaled[STACK_BOARD_MAX];
ivec window_dims;

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

  rdat->backs         = tex_from_file(rdat->rdr, "resources/cards/Card_Back-88x124.png");
  rdat->slot          = tex_from_file(rdat->rdr, "resources/cards/Card_Slot-88x124.png");
  rdat->piles         = tex_from_file(rdat->rdr, "resources/cards/Card_DeckA-88x140.png");
  rdat->locked_icon   = tex_from_file(rdat->rdr, "resources/cards/Card_locked_icon.png");
  rdat->suits[SUIT_H] = tex_from_file(rdat->rdr, "resources/cards/Hearts-88x124.png");
  rdat->suits[SUIT_D] = tex_from_file(rdat->rdr, "resources/cards/Diamonds-88x124.png");
  rdat->suits[SUIT_C] = tex_from_file(rdat->rdr, "resources/cards/Clubs-88x124.png");
  rdat->suits[SUIT_S] = tex_from_file(rdat->rdr, "resources/cards/Spades-88x124.png");

  graphics_resize(s, (ivec){s->sdata.window_w,s->sdata.window_h});
}

void
graphics_resize(solr *s, ivec new_window_dims) {
  window_dims = new_window_dims;
  ivec nwd = window_dims;
  for (int i = 0; i < STACK_BOARD_MAX; i++) {
    stack_areas_scaled[i] = ivec_div(ivec_mul(stack_areas[i], new_window_dims), ref_window_dims);
  }
  card_dims_scaled = ivec_div(ivec_mul(card_dims, new_window_dims), ref_window_dims);
  for (int i = 0; i < STACK_MAX; i++) {
    update_stack_dims(stack_from_id(s, i));
  }
}

void
graphics_finish(solr *s) {
  solr_rdr *rdat = &s->rdr;
  SDL_DestroyTexture(rdat->backs);
  SDL_DestroyTexture(rdat->piles);
  SDL_DestroyTexture(rdat->slot);
  for (size_t i = SUIT_BEG; i < SUIT_MAX; i++) {
    SDL_DestroyTexture(rdat->suits[i]);
  }
  SDL_DestroyRenderer(rdat->rdr);
  SDL_DestroyWindow(rdat->win);
  SDL_Quit();
}