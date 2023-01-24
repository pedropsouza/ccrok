#ifndef CROK_DATA_HG__
#define CROK_DATA_HG__

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"
#include "crok/config.h"
#include "crok/state.h"
#include "crok/unit.h"
#include "draw.h"
#include "fvec.h"
#include "net.h"
#include <stdint.h>

struct crok_setup_data {
  int width, height, servermode;
  char name[CROK_PLAYER_NAME_MAXLEN];
  struct sockaddr_in saddr;
};

struct crok_ui_text {
  SDL_Texture *tex;
  SDL_Rect dst;
};

struct crok_ui_data {
  TTF_Font *font;
  uint32_t baseline_ptsize;
  SDL_Color clr;
  struct crok_ui_text nameplates[4];
  struct crok_ui_text score;
  struct crok_ui_text round;
  struct crok_ui_text gamestage;
};

struct crok_render_data {
  SDL_Window *win;
  SDL_Renderer *rdr;
  struct crok_render_board board;
  struct crok_ui_data ui;
  int w, h, servermode;
  double a;
};

struct crok_data {
  struct crok_state crok;
  struct crok_board board;
  struct crok_render_data renderdata;
  struct crok_net_context net;
  size_t my_player_id;
  uint64_t ticknum;

  int width, height, servermode;
};

struct crok_data crok_game_init(struct crok_setup_data sdata);
void crok_game_finish(struct crok_data *d);
struct ivec crok_screen_to_board(struct crok_render_data *rd, struct fvec v);
struct fvec crok_board_to_screen(struct crok_render_data *rd, struct ivec v);

struct crok_ui_text crok_ui_new_text(SDL_Renderer *rdr, struct crok_ui_data *ui, char *text);
void crok_ui_free_text(struct crok_ui_text *t);

void crok_ui_render(struct crok_data *d);

#endif // CROK_DATA_HG__
