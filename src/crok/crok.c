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
      safe_calloc(sizeof(struct crok_data), "setup persistant_data alloc");
  struct crok_data *d = (struct crok_data *)s->persistant_data;
  struct crok_setup_data *setdat = (struct crok_setup_data *)s->setup_data;
  struct crok_render_data *rdat = &d->renderdata;

  rdat->w = setdat->width;
  rdat->h = setdat->height;
  rdat->a = 0.0;
  rdat->servermode = setdat->servermode;
  if (!rdat->servermode) {
    SDL_Init(SDL_INIT_VIDEO);
    rdat->win = SDL_CreateWindow("ccrok", SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED, rdat->w, rdat->h, 0);
    rdat->rdr = SDL_CreateRenderer(rdat->win, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(rdat->rdr, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(rdat->rdr, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(rdat->rdr);

    // modified dimensions maybe
    SDL_GetRendererOutputSize(rdat->rdr, &rdat->w, &rdat->h);
  }
  d->crok = crok_state_init(d);

  int smaller = (rdat->w > rdat->h) ? rdat->h : rdat->w;
  d->rboard = crok_board_render_init(rdat->rdr, &d->crok.board, rdat->w / 2,
                                     rdat->h / 2, smaller * 0.9);
  rdat->rboard = &d->rboard;

  crok_player_new(&d->crok, CROK_PLAYER_TEAM1, crok_local_input,
                  "local player");
  assert(crok_player_isvalid(&d->crok.players[0]));
  d->crok.active_player = 0;

  d->crok.board.discs[0] = crok_disc_init(&d->crok, 0);

  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "setup finished"};
}

struct sched_trans crok_cleanup(struct scheduler *s) {
  struct crok_data *d = (struct crok_data *)s->persistant_data;
  struct crok_render_data *r = &d->renderdata;

  crok_state_finish(&d->crok);
  if (!r->servermode) {
    SDL_DestroyRenderer(r->rdr);
    SDL_DestroyWindow(r->win);
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
  if (num_ticks * step_ns >= 10 * (long)1e9) {
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

  struct crok_disc *td = cs->board.discs;
  struct crok_player *tp = cs->players;

  struct ivec target = {.x = tp->cursor_rect.x, .y = tp->cursor_rect.y};
  struct ivec force = ivec_sub(target, td->pb.pos);
  // force = ivec_div(force, (struct ivec){.x = 1, .y = 1});
  ivec_print(force, "force");
  if (ivec_mag(force) > 10000) {
    goto forcetoobig;
  }
  physbody_accel(&td->pb,
                 ivec_div(force, (struct ivec){.x = CROK_TPS, .y = CROK_TPS}));
forcetoobig:

  for (size_t i = 0; i < 4; i++) {
    struct crok_player *player = &d->crok.players[i];
    if (crok_player_isvalid(player)) {
      crok_player_tick(player, &d->crok);
    }
  }

  for (size_t i = 0; i < CROK_DISCS_TOTAL; i++) {
    struct crok_disc *disc = &d->crok.board.discs[i];
    if (crok_disc_isvalid(disc)) {
      crok_disc_tick(disc, &d->crok);
    }
  }

  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "fixed call"};
}

void crok_render(struct scheduler *s, double a) {
  struct crok_data *d = (struct crok_data *)s->persistant_data;
  struct crok_render_data *r = &d->renderdata;

  SDL_SetRenderDrawColor(r->rdr, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(r->rdr);

  SDL_RenderCopy(r->rdr, d->rboard.tex, NULL, &d->rboard.dest);
  for (size_t i = 0; i < 4; i++) {
    struct crok_player *player[2] = {&d->crok.players[i], NULL};
    if (crok_player_isvalid(player[0])) {
      crok_player_render(player, r);
    }
  }

  for (size_t i = 0; i < CROK_DISCS_TOTAL; i++) {
    struct crok_disc *disc[2] = {&d->crok.board.discs[i], NULL};
    if (crok_disc_isvalid(disc[0])) {
      crok_disc_render(disc, r);
    }
  }

  // triggers the double buffers for multiple rendering
  SDL_RenderPresent(r->rdr);
}
