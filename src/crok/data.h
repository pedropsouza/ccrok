#ifndef CROK_DATA_HG__
#define CROK_DATA_HG__

#include "SDL2/SDL.h"
#include "crok/config.h"
#include "draw.h"
#include "fvec.h"
#include "input.h"
#include <stdint.h>

#define CROK_TEAM1 (1 << 0)
#define CROK_TEAM2 (1 << 1)
#define CROK_PLAYER_VALID (1 << 3)

#define CROK_DISC_VALID (1 << 0)
#define CROK_DISC_FALLEN (1 << 1)

enum crok_gamestage {
  CROK_GAMESTART = 0,
  CROK_SIDESEL,
  CROK_ROUNDSTART,
  CROK_SHOT,
  CROK_VALID,
  CROK_FOUL,
  CROK_TALLY,
  CROK_ROUNDEND,
  CROK_ERROR
};

struct crok_setup_data {
  int width, height;
};

struct crok_player {
  uint8_t id;
  char name[CROK_NAMELEN_MAX];
  uint8_t flags;
  uint32_t score;
};

struct crok_disc {
  uint8_t id;
  struct fvec pos;
  struct fvec vel;
  struct fvec acc;
  uint8_t owner_id;
  uint8_t flags;
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
  SDL_Texture *tex;
  SDL_Rect dest;
  int width, height;
  struct crok_state crok;
  struct crok_input_context input;
};

struct crok_state crok_state_init(struct crok_data *appdata);

#endif // CROK_DATA_HG__
