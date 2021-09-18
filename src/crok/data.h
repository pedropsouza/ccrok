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
  SDL_Texture *tex;
};

struct crok_board {
  SDL_Texture *tex;
  SDL_Rect dest;
  int arena_radius;
  int ditch_width;
  int hole_radius;
};

struct crok_state {
  size_t active_player;
  // either 2 or 4, as 2 sides
  struct crok_player players[4];
  struct crok_disc discs[CROK_DISCS_TOTAL];
  struct fvec posts[8];
  unsigned int round;
  int overall_score;
  enum crok_gamestage gamestage;
  struct crok_board board;
};

struct crok_data {
  SDL_Window *window;
  SDL_Renderer *renderer;
  int width, height, servermode;
  struct crok_state crok;
  struct crok_input_context input;
};

struct crok_state crok_state_init(struct crok_data *appdata);
void crok_state_finish(struct crok_state *cs);

#endif // CROK_DATA_HG__
