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

struct crok_state crok_state_init(struct crok_data *appdata) {
  struct crok_state s = {0};
  s.round = 1;
  s.gamestage = CROK_GAMESTART;
  s.overall_score = 0;

  struct crok_data *d = appdata;

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

struct sched_trans crok_setup(struct scheduler *s) {
  s->persistant_data =
      safe_malloc(sizeof(struct crok_data), "setup persistant_data alloc");
  struct crok_data *d = (struct crok_data *)s->persistant_data;
  struct crok_setup_data *setdat = (struct crok_setup_data *)s->setup_data;

  int width = setdat->width, height = setdat->height;
  SDL_Init(SDL_INIT_VIDEO);
  d->window = SDL_CreateWindow("ccrok", SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED, width, height, 0);

  d->renderer = SDL_CreateRenderer(d->window, -1, SDL_RENDERER_ACCELERATED);
  SDL_SetRenderDrawColor(d->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(d->renderer);

  // get the possibly modified dimensions
  SDL_GetRendererOutputSize(d->renderer, &width, &height);
  d->width = width;
  d->height = height;

  // creates a surface to load an image into the main memory
  SDL_Surface *surface;

  // please provide a path for your image
  surface = IMG_Load("resources/ul.png");

  // loads image to our graphics hardware memory.
  d->tex = SDL_CreateTextureFromSurface(d->renderer, surface);

  // clears main-memory
  SDL_FreeSurface(surface);

  // connects our texture with dest to control position
  SDL_QueryTexture(d->tex, NULL, NULL, &d->dest.w, &d->dest.h);

  // adjust height and width of our image box.
  d->dest.w /= 2;
  d->dest.h /= 2;

  // sets initial x-position of object
  d->dest.x = (width - d->dest.w) / 2;

  // sets initial y-position of object
  d->dest.y = (height - d->dest.h) / 2;

  d->crok = crok_state_init(d);
  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "setup finished"};
}

struct sched_trans crok_cleanup(struct scheduler *s) {
  struct crok_data *d = (struct crok_data *)s->persistant_data;

  // destroy texture and window
  SDL_DestroyTexture(d->tex);
  SDL_DestroyRenderer(d->renderer);
  SDL_DestroyWindow(d->window);

  d = NULL;
  free(s->persistant_data);
  s->persistant_data = NULL;

  SDL_Quit();
  return (struct sched_trans){.s = SCHED_HALT, .why = "cleanup finished"};
}

struct sched_trans crok_fixed(struct scheduler *s, long step_ns) {
  struct crok_data *d = (struct crok_data *)s->persistant_data;
  static long num_ticks = 0;
  num_ticks += 1;
  if (num_ticks * step_ns >= 10 * (long)1e9) {
    return (struct sched_trans){.s = SCHED_CLEANUP,
                                .why = "10 seconds elapsed"};
  }

  SDL_Event input;

  while (SDL_PollEvent(&input) > 0) {
    switch (input.type) {
    case SDL_QUIT:
      return (struct sched_trans){.s = SCHED_CLEANUP,
                                  .why = "user requested exit"};
      break;
    case SDL_MOUSEMOTION:
      d->dest.x = input.motion.x - d->dest.w / 2;
      d->dest.y = input.motion.y - d->dest.h / 2;
      break;
    }
  }

  SDL_RenderCopy(d->renderer, d->tex, NULL, &d->dest);
  SDL_RenderPresent(d->renderer);
  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "fixed call"};
}

void crok_render(struct scheduler *s, double a) {
  struct crok_data *d = (struct crok_data *)s->persistant_data;

  SDL_SetRenderDrawColor(d->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(d->renderer);

  SDL_RenderCopy(d->renderer, d->crok.board.tex, NULL, &d->crok.board.dest);
  SDL_RenderCopy(d->renderer, d->tex, NULL, &d->dest);

  // triggers the double buffers for multiple rendering
  SDL_RenderPresent(d->renderer);
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
