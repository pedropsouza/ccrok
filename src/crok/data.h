#ifndef CROK_DATA_HG__
#define CROK_DATA_HG__

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "crok/config.h"
#include "crok/state.h"
#include "crok/unit.h"
#include "draw.h"
#include "fvec.h"
#include "net.h"
#include <stdint.h>

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

struct crok_data {
  struct crok_state crok;
  struct crok_render_board rboard;
  struct crok_render_data renderdata;
  struct crok_net_context net;
  size_t my_player_id;
  uint64_t ticknum;

  int width, height, servermode;
};

struct ivec crok_screen_to_board(struct crok_render_data *rd, struct fvec v);
struct fvec crok_board_to_screen(struct crok_render_data *rd, struct ivec v);

#endif // CROK_DATA_HG__
