#ifndef CROK_DATA_HG__
#define CROK_DATA_HG__

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "crok/config.h"
#include "draw.h"
#include "fvec.h"
#include "input.h"
#include <stdint.h>

enum crok_gamestage {
  CROK_GAMESTART = 0,
  CROK_SIDESEL,
  CROK_ROUNDSTART,
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
  SDL_Renderer *r;
  struct crok_render_board *board;
  double a;
};

enum crok_player_flags {
  CROK_PLAYER_VALID = 1 << 0,
  CROK_PLAYER_TEAM1 = 1 << 1,
  CROK_PLAYER_TEAM2 = 1 << 2,
};

struct crok_player {
  uint8_t id;
  char name[CROK_NAMELEN_MAX];
  enum crok_player_flags flags;
  SDL_Rect cursor_rect;
  struct crok_input_context inputctx;
  crok_input_producer_fun inputf;
  uint32_t score;
};

enum crok_disc_flags { CROK_DISC_VALID = 1 << 0, CROK_DISC_FALLEN = 1 << 1 };

struct crok_disc {
  uint8_t id;
  struct fvec pos;
  struct fvec vel;
  struct fvec acc;
  uint8_t owner_id;
  enum crok_disc_flags flags;
  int radius;
  SDL_Texture *tex; // FIXME: discs shouldn't have to worry about their
                    // textures, Leave that to the renderdata or something
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

// units in pixels TODO: fix verbose names
struct crok_render_board {
  SDL_Texture *tex;
  SDL_Rect dest;
  double render_radius;
  double render_ditch_width;
  double render_hole_radius;
  double render_arena_radius;
  double render_scorearea_width;

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
  struct crok_render_board rboard;
};

struct crok_data {
  SDL_Window *window;
  SDL_Renderer *renderer;
  int width, height, servermode;
  struct crok_state crok;
  struct crok_input_context input;
};

struct crok_state crok_state_init(struct crok_data *appdata);
struct crok_board crok_board_calc(struct crok_data *appdata);
struct crok_render_board crok_board_render_init(struct crok_data *appdata,
                                                double size_mult);
void crok_board_render_finish(struct crok_render_board *rboard);
void crok_state_finish(struct crok_state *cs);

#endif // CROK_DATA_HG__
