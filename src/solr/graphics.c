#include "ivec.h"
#include "solr.h"
#include <raylib.h>
#include <stdio.h>

ivec ref_stack_areas[STACK_BOARD_MAX] = {
};

const ivec ref_window_dims = {.x = 850, .y = 478};
//const ivec card_dims = {.x = 88, .y = 124};
const ivec card_dims = {.x = 66, .y = 93};

static Texture2D
tex_from_file(const char* filename) {
  Image img = LoadImage(filename);
  ImageColorReplace(&img, (Color){00,128,128,255}, (Color){0,0,0,0});
  ImageColorReplace(&img, (Color){64,128,128,255}, (Color){0,0,0,0});
  ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  Texture2D tex = LoadTextureFromImage(img);
  UnloadImage(img);
  return tex;
}

void
graphics_init(solr *s) {
  solr_rdr *rdat = &s->rdr;
  memset(&rdat->stack_areas, 0, sizeof(rdat->stack_areas));

  InitWindow(s->sdata.window_w, s->sdata.window_h, "solr");

  int smaller = (s->sdata.window_w > s->sdata.window_h) ?
    s->sdata.window_h : s->sdata.window_w;

  rdat->backs         = tex_from_file("resources/cards/Card_Back-88x124.png");
  rdat->slot          = tex_from_file("resources/cards/Card_Slot-88x124.png");
  rdat->piles         = tex_from_file("resources/cards/Card_DeckA-88x140.png");
  rdat->locked_icon   = tex_from_file("resources/cards/Card_locked_icon.png");
  rdat->suits[SUIT_H] = tex_from_file("resources/cards/Hearts-88x124.png");
  rdat->suits[SUIT_D] = tex_from_file("resources/cards/Diamonds-88x124.png");
  rdat->suits[SUIT_C] = tex_from_file("resources/cards/Clubs-88x124.png");
  rdat->suits[SUIT_S] = tex_from_file("resources/cards/Spades-88x124.png");

  int cw = card_dims.x, ch = card_dims.y;
  ref_stack_areas[0]  = (ivec){.x = 12,                 .y = ch/4,     };
  ref_stack_areas[1]  = (ivec){.x = 12 + 1 * (12 + cw), .y = 140 + ch/4};
  ref_stack_areas[2]  = (ivec){.x = 12 + 2 * (12 + cw), .y = 140 + ch/4};
  ref_stack_areas[3]  = (ivec){.x = 12 + 3 * (12 + cw), .y = 140 + ch/4};
  ref_stack_areas[4]  = (ivec){.x = 12 + 4 * (12 + cw), .y = 140 + ch/4};
  ref_stack_areas[5]  = (ivec){.x = 12 + 5 * (12 + cw), .y = 140 + ch/4};
  ref_stack_areas[6]  = (ivec){.x = 12 + 6 * (12 + cw), .y = 140 + ch/4};
  ref_stack_areas[7]  = (ivec){.x = 12 + 7 * (12 + cw), .y = 140 + ch/4};
  ref_stack_areas[8]  = (ivec){.x = 12 + 4 * (12 + cw), .y = ch/4,     };
  ref_stack_areas[9]  = (ivec){.x = 12 + 5 * (12 + cw), .y = ch/4,     };
  ref_stack_areas[10] = (ivec){.x = 12 + 6 * (12 + cw), .y = ch/4,     };
  ref_stack_areas[11] = (ivec){.x = 12 + 7 * (12 + cw), .y = ch/4,     };
  ref_stack_areas[12] = (ivec){.x = 12 + 1 * (12 + cw), .y = ch/4,     };
  
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
    rd->stack_areas[i].width = rd->card_dims.x;
    rd->stack_areas[i].height = rd->card_dims.y;
    // printf("area %d -> %d,%d >< %d,%d\n",
    //   i,
    //   rd->stack_areas[i].x,
    //   rd->stack_areas[i].y,
    //   rd->stack_areas[i].w,
    //   rd->stack_areas[i].h
    // );
  }
  for (int i = 0; i < STACK_MAX; i++) {
    update_stack_dims(s, stack_from_id(s, i));
  }
}

void
graphics_finish(solr *s) {
  solr_rdr *rdat = &s->rdr;
  UnloadTexture(rdat->backs);
  UnloadTexture(rdat->piles);
  UnloadTexture(rdat->slot);
  for (size_t i = SUIT_BEG; i < SUIT_MAX; i++) {
    UnloadTexture(rdat->suits[i]);
  }
  CloseWindow();
}