#ifndef CROK_HG__
#define CROK_HG__

#include "fvec.h"
#include "memutil.h"
#include "sched.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

// SIZES currently in "board lengths lenghts",
// those being arena_radius + ditch_width
#define CROK_DISCS_PER_SIDE 12
#define CROK_DISCS_TOTAL (CROK_DISCS_PER_SIDE * 2)
#define CROK_DISC_SIZE 1.0 / 18.0
#define CROK_POST_SIZE 1.0 / 64.0
#define CROK_TEAM1 (1 << 0)
#define CROK_TEAM2 (1 << 1)
#define CROK_MAXNAMELEN 32

#define CROK_PLAYER_VALID (1 << 3)

#define CROK_DISC_VALID (1 << 0)
#define CROK_DISC_FALLEN (1 << 1)

struct crok_data;
struct input_data;

struct crok_state;
enum crok_gamestage;

struct crok_player {
  uint8_t id;
  char name[CROK_MAXNAMELEN];
  uint8_t flags;
  uint32_t score;
};
struct crok_player crok_player_init(uint8_t id, char name[CROK_MAXNAMELEN]);
uint8_t crok_player_team(struct crok_player *);
uint8_t crok_player_isvalid(struct crok_player *);
enum crok_gamestage crok_player_tick(struct crok_state *);

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
struct crok_disc crok_disc_init(struct crok_state *s, uint8_t owner);
uint8_t crok_disc_team(struct crok_disc *, struct crok_state *);
uint8_t crok_disc_isvalid(struct crok_disc *);
enum crok_gamestage crok_disc_tick(struct crok_disc *, struct crok_state *);
void crok_disc_render(struct crok_disc *);

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
struct crok_state crok_state_init(struct crok_data *appdata);

struct crok_data {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *tex;
  SDL_Rect dest;
  int width, height;
  struct crok_state crok;
};

struct crok_setup_data {
  int width, height;
};

struct sched_trans crok_setup(struct scheduler *s);
struct sched_trans crok_cleanup(struct scheduler *s);
struct sched_trans crok_fixed(struct scheduler *s, long step_ns);
void crok_render(struct scheduler *s, double a);

#endif // CROK_HG__
