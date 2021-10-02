#include "crok/data.h"

struct crok_state crok_state_init(struct crok_data *appdata) {
  struct crok_state s = {0};
  s.round = 1;
  s.gamestage = CROK_GAMESTART;
  s.overall_score = 0;

  struct crok_data *d = appdata;

  if (appdata->servermode) {
    return s;
  }

  return s;
}

void crok_calc_scale(struct crok_data *appdata) {
  struct crok_board *b = &appdata->crok.board;
  int smaller_dim =
      (appdata->width > appdata->height) ? appdata->height : appdata->width;
  int bd = smaller_dim * 0.9;
  b->dest.w = bd;
  b->dest.h = bd;
  int bx = (appdata->width - bd) / 2;
  int by = (appdata->height - bd) / 2;
  b->dest.x = bx;
  b->dest.y = by;
  b->tex = SDL_CreateTexture(appdata->renderer, SDL_PIXELFORMAT_RGBA8888,
                             SDL_TEXTUREACCESS_TARGET, bd, bd);
  {
    int br = bd / 2.0;
    int o = br;
    s.board.ditch_units = CROK_DITCH_DIAM;
    s.board.hole_units = CROK_HOLE_DIAM;
    s.board.arena_units = UINT32_MAX - CROK_DITCH_DIAM;
    int ditchw = bd * (CROK_DITCH_SIZE / UINT32_MAX);
    int holer = bd * (CROK_HOLE_RADIUS / UINT32_MAX);
    int arenar = br - ditchw;
    int scoreareaw = (arenar - holer) / 3.0;
    SDL_SetRenderTarget(d->renderer, s.board.tex);
    SDL_RenderClear(d->renderer);
    SDL_SetRenderDrawColor(d->renderer, 0x28, 0x15, 0x04, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(d->renderer, o, o, br - 1);
    SDL_SetRenderDrawColor(d->renderer, 0xbf, 0x25, 0x11, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(d->renderer, o, o, arenar);

    // quadrant lines
    int a, b;
    a = ((arenar / 3) * 2 - 1) * sqrt(2.0) / 2.0;
    b = arenar * sqrt(2.0) / 2.0;

    SDL_SetRenderDrawColor(d->renderer, 0xc0, 0xc0, 0xc0, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(d->renderer, o + a, o + a, o + b, o + b);
    SDL_RenderDrawLine(d->renderer, o - a, o - a, o - b, o - b);
    SDL_RenderDrawLine(d->renderer, o + a, o - a, o + b, o - b);
    SDL_RenderDrawLine(d->renderer, o - a, o + a, o - b, o + b);

    SDL_SetRenderDrawColor(d->renderer, 0x60, 0x19, 0x0f, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(d->renderer, o, o, holer + scoreareaw * 2);
    SDL_SetRenderDrawColor(d->renderer, 0x30, 0x0c, 0x07, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(d->renderer, o, o, holer + scoreareaw * 1);
    SDL_SetRenderDrawColor(d->renderer, 0x50, 0x14, 0x01, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(d->renderer, o, o, holer);

    // posts
    double postdist = holer + scoreareaw;
    double postr = (arenar + ditchw) * CROK_POST_SIZE;
    double step = atan(1.0);    // pi/4
    double offset = step / 2.0; // pi/8
    SDL_SetRenderDrawColor(d->renderer, 0xc0, 0xc0, 0xc0, SDL_ALPHA_OPAQUE);
    for (uint8_t i = 0; i < 8; i++) {
      double a = offset + step * i;
      struct fvec xy = fvec_xy(postdist * cos(a), postdist * sin(a));
      s.posts[i] = xy;
      SDL_RenderFillCircle(d->renderer, o + xy.x, o + xy.y, postr);
    }
  }
  SDL_SetRenderTarget(d->renderer, NULL);

  int dsize = (s.board.arena_radius + s.board.ditch_width) * CROK_DISC_SIZE;
  int o = dsize / 2;

  s.disc_tex = SDL_CreateTexture(d->renderer, SDL_PIXELFORMAT_RGBA8888,
                                 SDL_TEXTUREACCESS_TARGET, dsize, dsize);
  SDL_SetRenderTarget(d->renderer, s.disc_tex);
  {
    SDL_SetRenderDrawColor(d->renderer, 0xcc, 0xcc, 0xcc, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(d->renderer, o, o, dsize);
  }
  SDL_SetRenderTarget(d->renderer, NULL);
}

void crok_state_finish(struct crok_state *cs) {
  if (cs->board.tex) {
    SDL_DestroyTexture(cs->board.tex);
    SDL_DestroyTexture(cs->disc_tex);
  }
}
