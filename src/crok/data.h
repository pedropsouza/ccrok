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
  CROK_LOBBY = 0,
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
  struct sockaddr_in saddr;
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

#define CROK_STATE_SERLEN                                                      \
  (sizeof(int8_t) * 2 + CROK_PLAYER_SERLEN * 4 +                               \
   CROK_DISC_SERLEN * CROK_DISCS_TOTAL + sizeof(uint32_t) * 2 +                \
   sizeof(uint16_t))
struct crok_state {
  int8_t active_player, total_players;
  // either 2, or 4 as 2 sides
  struct crok_player players[4];
  struct crok_disc discs[CROK_DISCS_TOTAL];
  uint32_t round;
  uint32_t overall_score;
  enum crok_gamestage gamestage;
  struct crok_board board;
};

struct crok_data {
  struct crok_state crok;
  struct crok_render_board rboard;
  struct crok_render_data renderdata;
  struct crok_input_context input;
  size_t my_player_id;

  int width, height, servermode, sockfd;
  struct sockaddr_in saddr;
};

struct crok_state crok_state_init();
struct crok_board crok_board_calc();
struct crok_render_board crok_board_render_init(SDL_Renderer *renderer,
                                                const struct crok_board *subj,
                                                int pos_x, int pos_y,
                                                int px_size);
void crok_board_render_finish(struct crok_render_board *rboard);
struct ivec crok_screen_to_board(struct crok_render_data *rd, struct fvec v);
struct fvec crok_board_to_screen(struct crok_render_data *rd, struct ivec v);

char *crok_state_serialize(const struct crok_state *st);
struct crok_state crok_state_deserialize(char buf[CROK_STATE_SERLEN]);

#endif // CROK_DATA_HG__
