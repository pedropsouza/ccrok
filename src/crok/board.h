#ifndef CROK_BOARD_HG__
#define CROK_BOARD_HG__

#include "crok/config.h"
#include "draw.h"
#include "fvec.h"
#include <SDL.h>
#include <stdint.h>

// units are in 1/UINT32_MAX whole boards,
struct crok_board {
  uint32_t arena_units;
  uint32_t ditch_units;
  uint32_t hole_units;
  uint32_t scorearea_units; // width of a _single_ scoring "strip" (out of 3)
  uint32_t post_radius;
  uint32_t disc_radius;

  struct fvec posts[8];
};

// units in pixels
struct crok_render_board {
  SDL_Texture *tex;
  SDL_Rect dest;
  double radius;
  double diam;
  double ditch_width;
  double hole_diam;
  double arena_diam;
  double scorearea_width;

  SDL_Texture *disc_tex;
  double disc_diam;
};

struct crok_board crok_board_calc();
struct crok_render_board crok_board_render_init(SDL_Renderer *renderer,
                                                const struct crok_board *subj,
                                                int pos_x, int pos_y,
                                                int px_size);
void crok_board_render_finish(struct crok_render_board *rboard);

#endif // CROK_BOARD_HG__
