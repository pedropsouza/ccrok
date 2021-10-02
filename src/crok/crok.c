#include "crok.h"

struct crok_input_packet crok_local_input(struct crok_input_context *ic,
                                          void *extra) {
  SDL_Event input;

  if (SDL_PollEvent(&input) > 0) {
    switch (input.type) {
    case SDL_QUIT:
      return crok_input_quit_packet();
      break;
    case SDL_MOUSEMOTION:
      return crok_input_mouse_packet(input.motion.x, input.motion.y);
      break;
    default:
      return (struct crok_input_packet){0};
      break;
    }
  }
  return crok_input_end_packet();
}

struct sched_trans crok_setup(struct scheduler *s) {
  s->persistant_data =
      safe_malloc(sizeof(struct crok_data), "setup persistant_data alloc");
  struct crok_data *d = (struct crok_data *)s->persistant_data;
  struct crok_setup_data *setdat = (struct crok_setup_data *)s->setup_data;

  if (!setdat->servermode) {
    int width = setdat->width, height = setdat->height;
    SDL_Init(SDL_INIT_VIDEO);
    d->window = SDL_CreateWindow("ccrok", SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED, width, height, 0);

    d->renderer = SDL_CreateRenderer(d->window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(d->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(d->renderer);

    // modified dimensions maybe
    SDL_GetRendererOutputSize(d->renderer, &width, &height);
    d->width = width;
    d->height = height;
  }

  d->crok = crok_state_init(d);
  crok_player_new(&d->crok, CROK_PLAYER_TEAM1, crok_local_input,
                  "local player");
  assert(crok_player_isvalid(&d->crok.players[0]));
  d->crok.active_player = 0;

  d->crok.discs[0] = crok_disc_init(&d->crok, 0);

  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "setup finished"};
}

struct sched_trans crok_cleanup(struct scheduler *s) {
  struct crok_data *d = (struct crok_data *)s->persistant_data;
  struct crok_setup_data *setdat = (struct crok_setup_data *)s->setup_data;

  crok_state_finish(&d->crok);
  if (!setdat->servermode) {
    SDL_DestroyRenderer(d->renderer);
    SDL_DestroyWindow(d->window);
    SDL_Quit();
  }

  d = NULL;
  free(s->persistant_data);
  s->persistant_data = NULL;

  return (struct sched_trans){.s = SCHED_HALT, .why = "cleanup finished"};
}

struct sched_trans crok_fixed(struct scheduler *s, long step_ns) {
  struct crok_data *d = (struct crok_data *)s->persistant_data;
  struct crok_state *cs = (struct crok_state *)&d->crok;
  static long num_ticks = 0;
  num_ticks += 1;
  if (num_ticks * step_ns >= 1000 * (long)1e9) {
    return (struct sched_trans){.s = SCHED_CLEANUP,
                                .why = "10 seconds elapsed"};
  }

  struct crok_player *p = &cs->players[cs->active_player];
  if (!crok_player_isvalid(p)) {
    cs->gamestage = CROK_ERROR;
    return (struct sched_trans){.s = SCHED_ERR, .why = "invalid active player"};
  }
  enum crok_gamestage gst = crok_player_tick(p, cs);
  switch (gst) {
  case CROK_LEAVE:
    return (struct sched_trans){.s = SCHED_CLEANUP,
                                .why = "user requested quit"};
    break;
  default:
    break;
  }

  cs->discs[0].pos.x = cs->players[0].cursor_rect.x;
  cs->discs[0].pos.y = cs->players[0].cursor_rect.y;

  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "fixed call"};
}

void crok_render(struct scheduler *s, double a) {
  struct crok_data *d = (struct crok_data *)s->persistant_data;
  struct crok_render_data rd;
  rd.r = d->renderer;
  rd.board = &d->crok.board;
  rd.a = a;

  SDL_SetRenderDrawColor(d->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(d->renderer);

  SDL_RenderCopy(d->renderer, d->crok.board.tex, NULL, &d->crok.board.dest);
  for (size_t i = 0; i < 4; i++) {
    struct crok_player *player[2] = {&d->crok.players[i], NULL};
    if (crok_player_isvalid(player[0])) {
      crok_player_render(player, &rd);
    }
  }

  for (size_t i = 0; i < CROK_DISCS_TOTAL; i++) {
    struct crok_disc *disc[2] = {&d->crok.discs[i], NULL};
    if (crok_disc_isvalid(disc[0])) {
      crok_disc_render(disc, &rd);
    }
  }

  // triggers the double buffers for multiple rendering
  SDL_RenderPresent(d->renderer);
}
