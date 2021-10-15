#include "crok/data.h"

struct crok_state crok_state_init(struct crok_data *appdata) {
  struct crok_state s = {0};
  s.round = 1;
  s.gamestage = CROK_GAMESTART;
  s.overall_score = 0;

  struct crok_data *d = appdata;
  crok_board_calc(d);

  if (d->servermode) {
    return s;
  }

  crok_board_render_init(d, 0.9);

  return s;
}

struct crok_board crok_board_calc(struct crok_data *appdata) {
  struct crok_data *d = appdata;
  struct crok_board b = {0};

  b.ditch_units = CROK_DITCH_DIAM;
  b.hole_units = CROK_HOLE_DIAM;
  b.arena_units = CROK_BOARD_DIAM - CROK_DITCH_DIAM;
  b.scorearea_units = (b.arena_units - b.hole_units) / 3;
  b.post_radius = (CROK_POST_DIAM / CROK_BOARD_DIAM);

  // TODO: make this all fixed point, including the post positions
  uint32_t postdist = b.hole_units + b.scorearea_units;
  double step = atan(1.0);    // pi/4
  double offset = step / 2.0; // pi/8
  printf("postd == %ld\n", (long)postdist);
  for (uint8_t i = 0; i < 8; i++) {
    double a = offset + step * i;
    struct fvec xy = fvec_xy(postdist * cos(a), postdist * sin(a));
    b.posts[i] = xy;
    printf("i == %d, xy == (%f,%f), (%f,%f)\n", i, xy.x, xy.y, b.posts[i].x,
           b.posts[i].y);
  }
  return b;
}

struct crok_render_board crok_board_render_init(struct crok_data *appdata,
                                                double size_mult) {
  struct crok_data *d = appdata;
  struct crok_board *b = &d->crok.board;
  struct crok_render_board rb = {0};

  // Loose, screen aspect ratio
  double smaller_dim = (d->width > d->height) ? d->height : d->width;
  double bd = smaller_dim * size_mult;
  uint32_t bx = (d->width - bd) / 2;
  uint32_t by = (d->height - bd) / 2;
  rb.dest.w = bd;
  rb.dest.h = bd;
  rb.dest.x = bx;
  rb.dest.y = by;

  // Loose, render specific
  double rr = bd / 2.0;
  rb.render_radius = rr;
  double o = rr;
  rb.render_ditch_width = bd * (b->ditch_units / CROK_BOARD_DIAM);
  rb.render_hole_radius = bd * (b->hole_units / CROK_BOARD_DIAM);
  rb.render_arena_radius = bd * (b->arena_units / CROK_BOARD_DIAM);
  rb.render_scorearea_width = bd * (b->scorearea_units / CROK_BOARD_DIAM);

  if (rb.tex == NULL) {
    rb.tex = SDL_CreateTexture(d->renderer, SDL_PIXELFORMAT_RGBA8888,
                               SDL_TEXTUREACCESS_TARGET, bd, bd);
  }
  {
    SDL_SetRenderTarget(d->renderer, rb.tex);

    double br = rb.render_radius;
    double o = br;
    double ditchw = rb.render_ditch_width;
    double holer = rb.render_hole_radius;
    double arenar = rb.render_arena_radius;
    double scoreareaw = rb.render_scorearea_width;

    SDL_RenderClear(d->renderer);
    SDL_SetRenderDrawColor(d->renderer, 0x28, 0x15, 0x04, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(d->renderer, o, o, br - 1);
    SDL_SetRenderDrawColor(d->renderer, 0xbf, 0x25, 0x11, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(appdata->renderer, o, o, arenar);

    // quadrant lines
    uint32_t x, y;
    x = ((arenar / 3) * 2 - 1) * sqrt(2.0) / 2.0;
    y = arenar * sqrt(2.0) / 2.0;

    SDL_SetRenderDrawColor(d->renderer, 0xc0, 0xc0, 0xc0, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(d->renderer, o + x, o + x, o + y, o + y);
    SDL_RenderDrawLine(d->renderer, o - x, o - x, o - y, o - y);
    SDL_RenderDrawLine(d->renderer, o + x, o - x, o + y, o - y);
    SDL_RenderDrawLine(d->renderer, o - x, o + x, o - y, o + y);

    SDL_SetRenderDrawColor(d->renderer, 0x60, 0x19, 0x0f, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(d->renderer, o, o, holer + scoreareaw * 2);
    SDL_SetRenderDrawColor(d->renderer, 0x30, 0x0c, 0x07, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(d->renderer, o, o, holer + scoreareaw * 1);
    SDL_SetRenderDrawColor(d->renderer, 0x50, 0x14, 0x01, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(d->renderer, o, o, holer);

    // posts
    float postr = (arenar + ditchw) * (CROK_POST_DIAM / CROK_BOARD_DIAM);
    SDL_SetRenderDrawColor(d->renderer, 0xc0, 0xc0, 0xc0, SDL_ALPHA_OPAQUE);
    for (uint8_t i = 0; i < 8; i++) {
      struct fvec pos = b->posts[i];
      printf("pos (%f,%f)\n", pos.x, pos.y);
      SDL_RenderFillCircle(d->renderer, o + pos.x, o + pos.y, postr);
    }

    SDL_SetRenderTarget(d->renderer, NULL);
  }
  {
    double dsize = bd * (CROK_DISC_DIAM / CROK_BOARD_DIAM);
    rb.disc_radius = dsize;

    rb.disc_tex = SDL_CreateTexture(d->renderer, SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET, dsize, dsize);
    SDL_SetRenderTarget(d->renderer, rb.disc_tex);
    {
      SDL_SetRenderDrawColor(d->renderer, 0xcc, 0xcc, 0xcc, SDL_ALPHA_OPAQUE);
      SDL_RenderFillCircle(d->renderer, o, o, dsize);
    }
    SDL_SetRenderTarget(d->renderer, NULL);
  }
  return rb;
}

void crok_board_render_finish(struct crok_render_board *rboard) {
  if (rboard->tex != NULL) {
    SDL_DestroyTexture(rboard->tex);
    SDL_DestroyTexture(rboard->disc_tex);
  }
}

void crok_state_finish(struct crok_state *cs) {
  crok_board_render_finish(&cs->rboard);
}
