#include "crok.h"

int SDL_RenderDrawCircle(SDL_Renderer *renderer, int x, int y, int radius);
int SDL_RenderFillCircle(SDL_Renderer *renderer, int x, int y, int radius);

struct crok_player crok_player_init(uint8_t id, char name[CROK_MAXNAMELEN]) {
  struct crok_player p = {};
  memcpy(p.name, name, CROK_MAXNAMELEN - 1);
  p.name[CROK_MAXNAMELEN - 1] = '\0';
  p.flags = 0;
  p.id = id;
  p.score = 0;
  return p;
}
uint8_t crok_player_team(struct crok_player *p) {
  return p->flags & (CROK_TEAM1 | CROK_TEAM2);
}
uint8_t crok_player_isvalid(struct crok_player *p) {
  return p->flags & CROK_PLAYER_VALID;
}
enum crok_gamestage crok_player_tick(struct crok_state *s) {
  // TODO
  return s->gamestage;
}

struct crok_disc crok_disc_init(struct crok_state *s, uint8_t owner) {
  struct crok_disc d = {.id = 0, // FIXME,
                        .owner_id = owner,
                        .pos = fvec_xy(0, 0),
                        .vel = fvec_xy(0, 0),
                        .acc = fvec_xy(0, 0),
                        .radius = CROK_DISC_SIZE,
                        .flags = 0};
  return d;
}
uint8_t crok_disc_team(struct crok_disc *d, struct crok_state *s) {
  return crok_player_team(&s->players[d->owner_id]);
}
uint8_t crok_disc_isvalid(struct crok_disc *d) {
  return d->flags & CROK_DISC_VALID;
}
enum crok_gamestage crok_disc_tick(struct crok_disc *d, struct crok_state *s) {
  // TODO
  return s->gamestage;
}

void crok_disc_render(struct crok_disc *d) {}

struct crok_state crok_state_init(struct app_data *appdata) {
  struct crok_state s = {0};
  s.round = 1;
  s.gamestage = CROK_GAMESTART;
  s.overall_score = 0;

  struct app_data *d = appdata;
  s.app_data = d;

  int smaller_dim = (d->width > d->height) ? d->height : d->width;
  int bd = smaller_dim * 0.9;
  s.board.dest.w = bd;
  s.board.dest.h = bd;
  int bx = (d->width - bd) / 2;
  int by = (d->height - bd) / 2;
  s.board.dest.x = bx;
  s.board.dest.y = by;

  s.board.tex = SDL_CreateTexture(d->renderer, SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_TARGET, bd, bd);
  {
    int br = bd / 2.0;
    int o = br;
    int ditchw = br / 12.0;
    int holer = br / 16.0;
    int arenar = br - ditchw;
    int scoreareaw = (arenar - holer) / 3.0;
    s.board.ditch_width = ditchw;
    s.board.arena_radius = arenar;
    s.board.hole_radius = holer;
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
  return s;
}
enum crok_gamestage crock_fixed(struct crok_state *s) { return s->gamestage; }
void crok_render(struct crok_state *s) {
  SDL_RenderCopy(s->app_data->renderer, s->board.tex, NULL, &s->board.dest);
}

int SDL_RenderDrawCircle(SDL_Renderer *renderer, int x, int y, int radius) {
  int offsetx, offsety, d;
  int status;

  offsetx = 0;
  offsety = radius;
  d = radius - 1;
  status = 0;

  while (offsety >= offsetx) {
    status += SDL_RenderDrawPoint(renderer, x + offsetx, y + offsety);
    status += SDL_RenderDrawPoint(renderer, x + offsety, y + offsetx);
    status += SDL_RenderDrawPoint(renderer, x - offsetx, y + offsety);
    status += SDL_RenderDrawPoint(renderer, x - offsety, y + offsetx);
    status += SDL_RenderDrawPoint(renderer, x + offsetx, y - offsety);
    status += SDL_RenderDrawPoint(renderer, x + offsety, y - offsetx);
    status += SDL_RenderDrawPoint(renderer, x - offsetx, y - offsety);
    status += SDL_RenderDrawPoint(renderer, x - offsety, y - offsetx);

    if (status < 0) {
      status = -1;
      break;
    }

    if (d >= 2 * offsetx) {
      d -= 2 * offsetx + 1;
      offsetx += 1;
    } else if (d < 2 * (radius - offsety)) {
      d += 2 * offsety - 1;
      offsety -= 1;
    } else {
      d += 2 * (offsety - offsetx - 1);
      offsety -= 1;
      offsetx += 1;
    }
  }

  return status;
}

int SDL_RenderFillCircle(SDL_Renderer *renderer, int x, int y, int radius) {
  int offsetx, offsety, d;
  int status;

  offsetx = 0;
  offsety = radius;
  d = radius - 1;
  status = 0;

  while (offsety >= offsetx) {

    status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx,
                                 x + offsety, y + offsetx);
    status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety,
                                 x + offsetx, y + offsety);
    status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety,
                                 x + offsetx, y - offsety);
    status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx,
                                 x + offsety, y - offsetx);

    if (status < 0) {
      status = -1;
      break;
    }

    if (d >= 2 * offsetx) {
      d -= 2 * offsetx + 1;
      offsetx += 1;
    } else if (d < 2 * (radius - offsety)) {
      d += 2 * offsety - 1;
      offsety -= 1;
    } else {
      d += 2 * (offsety - offsetx - 1);
      offsety -= 1;
      offsetx += 1;
    }
  }

  return status;
}
