#include "crok/data.h"

// sets up most things, except the networking subsystem
struct crok_data
crok_game_init(struct crok_setup_data sdata) {
   struct crok_data d = {
    .width = sdata.width,
    .height = sdata.height,
    .servermode = sdata.servermode,
    .crok = crok_state_init(),
    .board = crok_board_calc()
  };

  if (!d.servermode) {
    struct crok_render_data *rdat = &d.renderdata;

    rdat->w = d.width;
    rdat->h = d.height;
    rdat->a = 0.0;
    rdat->servermode = d.servermode;
    SDL_Init(SDL_INIT_VIDEO);
    rdat->win = SDL_CreateWindow("ccrok", SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED, rdat->w, rdat->h, 0);
    rdat->rdr = SDL_CreateRenderer(rdat->win, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(rdat->rdr, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(rdat->rdr, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(rdat->rdr);

    // modified dimensions maybe
    SDL_GetRendererOutputSize(rdat->rdr, &rdat->w, &rdat->h);

    int smaller = (rdat->w > rdat->h) ? rdat->h : rdat->w;
    rdat->board = crok_board_render_init(rdat->rdr, &d.board, rdat->w / 2,
                                       rdat->h / 2, smaller * 0.9);
  }
  return d;
}

void
crok_game_finish(struct crok_data *d) {
  if (!d->servermode) {
    struct crok_render_data *r = &d->renderdata;
    crok_board_render_finish(&d->renderdata.board);
    SDL_DestroyRenderer(r->rdr);
    SDL_DestroyWindow(r->win);
    SDL_Quit();
  }
}

struct ivec
crok_screen_to_board(struct crok_render_data *rd, struct fvec v) {
  double ox = ((double)v.x - rd->board.dest.x) / rd->board.diam;
  double oy = ((double)v.y - rd->board.dest.y) / rd->board.diam;
  double fx = ox * CROK_BOARD_DIAM;
  double fy = oy * CROK_BOARD_DIAM;
  return (struct ivec){.x = fx, .y = fy};
}

struct fvec
crok_board_to_screen(struct crok_render_data *rd, struct ivec v) {
  double fx = (double)v.x / CROK_BOARD_DIAM;
  double fy = (double)v.y / CROK_BOARD_DIAM;
  double ox = (fx * rd->board.diam) + rd->board.dest.x;
  double oy = (fy * rd->board.diam) + rd->board.dest.y;
  return (struct fvec){.x = ox, .y = oy};
}
