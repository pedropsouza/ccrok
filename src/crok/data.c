#include "crok/data.h"

struct crok_state crok_state_init(struct crok_data *appdata) {
  struct crok_state s = {0};
  s.round = 1;
  s.gamestage = CROK_GAMESTART;
  s.overall_score = 0;

  struct crok_data *d = appdata;
  s.board = crok_board_calc(d);

  if (d->servermode) {
    return s;
  }

  int w = d->width;
  int h = d->height;
  int smaller = (w > h) ? h : w;
  s.rboard = crok_board_render_init(d->renderer, &s.board, d->width / 2,
                                    d->height / 2, smaller * 0.9);
  return s;
}

struct crok_board crok_board_calc() {
  struct crok_board b = {0};

  b.ditch_units = CROK_DITCH_DIAM;
  b.hole_units = CROK_HOLE_DIAM;
  b.arena_units = CROK_BOARD_DIAM - CROK_DITCH_DIAM;
  b.scorearea_units = (b.arena_units - b.hole_units) / 3;
  b.post_radius = CROK_POST_DIAM;

  // TODO: make this all fixed point, including the post positions
  uint64_t postdist = b.hole_units + b.scorearea_units;
  double step = atan(1.0);    // pi/4
  double offset = step / 2.0; // pi/8
  for (uint8_t i = 0; i < 8; i++) {
    double a = offset + step * i;
    struct fvec xy = fvec_xy(postdist * cos(a), postdist * sin(a));
    b.posts[i] = xy;
  }
  return b;
}

struct crok_render_board crok_board_render_init(SDL_Renderer *renderer,
                                                const struct crok_board *subj,
                                                int pos_x, int pos_y,
                                                int px_size) {
  // !! don't forget to cast the numerator of the
  // fixed -> fractional conversion to double!
  const struct crok_board *b = subj;
  struct crok_render_board rb = {0};

  // Loose, screen aspect ratio
  long bd = px_size;
  PRINTLVAR(bd);
  rb.dest.w = bd;
  rb.dest.h = bd;
  rb.dest.x = pos_x - (bd / 2);
  rb.dest.y = pos_y - (bd / 2);
  PRINTLVAR((long)rb.dest.w);
  PRINTLVAR((long)rb.dest.h);
  PRINTLVAR((long)rb.dest.x);
  PRINTLVAR((long)rb.dest.y);

  // Loose, render specific
  double rr = bd / 2.0;
  rb.render_radius = rr;
  double o = rr;
  rb.render_ditch_width = rr * ((double)b->ditch_units / CROK_BOARD_DIAM);
  rb.render_hole_radius = rr * ((double)b->hole_units / CROK_BOARD_DIAM);
  rb.render_arena_radius = rr * ((double)b->arena_units / CROK_BOARD_DIAM);
  rb.render_scorearea_width =
      rr * ((double)b->scorearea_units / CROK_BOARD_DIAM);

  PRINTFVAR(rb.render_ditch_width);
  PRINTFVAR(rb.render_hole_radius);
  PRINTFVAR(rb.render_arena_radius);
  PRINTFVAR(rb.render_scorearea_width);

  rb.tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                             SDL_TEXTUREACCESS_TARGET, bd, bd);
  SDL_SetRenderTarget(renderer, rb.tex);
  {
    double ditchw = rb.render_ditch_width;
    double holer = rb.render_hole_radius;
    double arenar = rb.render_arena_radius;
    double scoreareaw = rb.render_scorearea_width;
    PRINTFVAR(o);
    PRINTFVAR(ditchw);
    PRINTFVAR(holer);
    PRINTFVAR(arenar);
    PRINTFVAR(scoreareaw);

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_TRANSPARENT);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0x28, 0x15, 0x04, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(renderer, o, o, rr - 1);
    SDL_SetRenderDrawColor(renderer, 0xbf, 0x25, 0x11, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(renderer, o, o, arenar);

    // quadrant lines
    double x, y;
    x = ((arenar / 3) * 2 - 1) * sqrt(2.0) / 2.0;
    y = arenar * sqrt(2.0) / 2.0;
    PRINTFVAR(x);
    PRINTFVAR(y);

    SDL_SetRenderDrawColor(renderer, 0xc0, 0xc0, 0xc0, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer, o + x, o + x, o + y, o + y);
    SDL_RenderDrawLine(renderer, o - x, o - x, o - y, o - y);
    SDL_RenderDrawLine(renderer, o + x, o - x, o + y, o - y);
    SDL_RenderDrawLine(renderer, o - x, o + x, o - y, o + y);

    SDL_SetRenderDrawColor(renderer, 0x60, 0x19, 0x0f, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(renderer, o, o, holer + scoreareaw * 2);
    SDL_SetRenderDrawColor(renderer, 0x30, 0x0c, 0x07, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(renderer, o, o, holer + scoreareaw * 1);
    SDL_SetRenderDrawColor(renderer, 0x50, 0x14, 0x01, SDL_ALPHA_OPAQUE);
    SDL_RenderFillCircle(renderer, o, o, holer);

    // posts
    double postr = rr * ((double)b->post_radius / CROK_BOARD_DIAM);
    SDL_SetRenderDrawColor(renderer, 0xc0, 0xc0, 0xc0, SDL_ALPHA_OPAQUE);
    for (uint8_t i = 0; i < 8; i++) {
      struct fvec pos = b->posts[i];
      pos.x = rr * ((double)pos.x / CROK_BOARD_DIAM);
      pos.y = rr * ((double)pos.y / CROK_BOARD_DIAM);
      SDL_RenderFillCircle(renderer, o + pos.x, o + pos.y, postr);
    }
  }
  SDL_SetRenderTarget(renderer, NULL);

  {
    double dsize = bd * ((double)CROK_DISC_DIAM / CROK_BOARD_DIAM);
    rb.disc_radius = dsize / 2;
    double o = dsize / 2;

    rb.disc_tex =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                          SDL_TEXTUREACCESS_TARGET, dsize + 1, dsize + 1);
    SDL_SetTextureBlendMode(rb.disc_tex, SDL_BLENDMODE_BLEND);
    const char *errs;
    if (strcmp((errs = SDL_GetError()), "") != 0) {
      printf("disc texture: %s\n", errs);
    }
    SDL_SetRenderTarget(renderer, rb.disc_tex);
    {
      SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_TRANSPARENT);
      SDL_RenderClear(renderer);
      SDL_SetRenderDrawColor(renderer, 0xcc, 0xcc, 0xcc, SDL_ALPHA_OPAQUE);
      SDL_RenderFillCircle(renderer, o, o, rb.disc_radius);
    }
    SDL_SetRenderTarget(renderer, NULL);
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
