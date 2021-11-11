#ifndef CROK_DATA_HG__
#define CROK_DATA_HG__

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "crok/config.h"
#include "crok/unit.h"
#include "draw.h"
#include "fvec.h"
#include "input.h"
#include <stdint.h>

enum crok_gamestage {
  CROK_GAMESTART = 0,
  CROK_SIDESEL,
  CROK_ROUNDSTART,
  CROK_AIM,
  CROK_SHOT,
  CROK_VALID,
  CROK_FOUL,
  CROK_TALLY,
  CROK_ROUNDEND,
  CROK_LEAVE,
  CROK_ERROR
};

struct crok_setup_data {
  int width, height, servermode;
};

struct crok_render_data {
  SDL_Window *win;
  SDL_Renderer *rdr;
  struct crok_render_board *rboard;
  int w, h, servermode;
  double a;
};

// units are in 1/UINT32_MAX whole boards,
struct crok_board {
  uint32_t arena_units;
  uint32_t ditch_units;
  uint32_t hole_units;
  uint32_t scorearea_units; // width of a _single_ scoring "strip" (out of 3)
  uint32_t post_radius;
  uint32_t disc_radius;

  struct fvec posts[8];
  struct crok_disc discs[CROK_DISCS_TOTAL];
};

// units in pixels
struct crok_render_board {
  SDL_Texture *tex;
  SDL_Rect dest;
  double radius;
  double ditch_width;
  double hole_radius;
  double arena_radius;
  double scorearea_width;

  SDL_Texture *disc_tex;
  double disc_radius;
};

struct crok_state {
  size_t active_player;
  // either 2 or 4, as 2 sides
  struct crok_player players[4];
  unsigned int round;
  int overall_score;
  enum crok_gamestage gamestage;
  struct crok_board board;
};

struct crok_data {
  struct crok_state crok;
  struct crok_render_board rboard;
  struct crok_render_data renderdata;
  struct crok_input_context input;
  size_t my_player_id;
};

struct crok_state crok_state_init(struct crok_data *appdata);
struct crok_board crok_board_calc();
struct crok_render_board crok_board_render_init(SDL_Renderer *renderer,
                                                const struct crok_board *subj,
                                                int pos_x, int pos_y,
                                                int px_size);
void crok_board_render_finish(struct crok_render_board *rboard);
void crok_state_finish(struct crok_state *cs);
struct ivec crok_screen_to_board(struct crok_render_data *rd, struct fvec v);
struct fvec crok_board_to_screen(struct crok_render_data *rd, struct ivec v);

#endif // CROK_DATA_HG__
