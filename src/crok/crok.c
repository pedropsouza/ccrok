#include "crok.h"

struct crok_input_packet crok_local_input(struct crok_input_context *ic,
                                          void *extra) {
#define CIP struct crok_input_packet

  SDL_Event input;

  if (SDL_PollEvent(&input) > 0) {
    switch (input.type) {
    case SDL_QUIT:
      return (CIP){.kind = CROK_INPUT_QUIT};
      break;
    case SDL_MOUSEMOTION:
      return (CIP){.pos = {.kind = CROK_INPUT_POS,
                           .pos = {.x = input.motion.x, .y = input.motion.y}}};
      break;
    case SDL_MOUSEBUTTONUP:
      break;
    case SDL_MOUSEBUTTONDOWN:
      break;
    default:
      return (CIP){0};
      break;
    }
  }
  return (CIP){.kind = CROK_INPUT_END};
#undef CIP
}

struct sched_trans crok_setup(struct scheduler *s) {
  s->persistent_data =
      safe_calloc(sizeof(struct crok_data), "setup persistent_data alloc");
  struct crok_data *d = (struct crok_data *)s->persistent_data;
  struct crok_setup_data *setdat = (struct crok_setup_data *)s->setup_data;

  d->crok = crok_state_init();

  // setup the network sockets first, if that fails
  // the cleanup is easier
  if (setdat->servermode) {
    struct sockaddr_in addrdat;
    int opt;
    setdat->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (setdat->sockfd == -1) {
      perror("Couldn't setup a network socket");
      free(s->persistent_data);
      s->persistent_data = NULL;
      return (struct sched_trans){.s = SCHED_HALT, .why = "setup error"};
    }
  } else {
    struct crok_render_data *rdat = &d->renderdata;

    rdat->w = setdat->width;
    rdat->h = setdat->height;
    rdat->a = 0.0;
    rdat->servermode = setdat->servermode;
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
    d->rboard = crok_board_render_init(rdat->rdr, &d->crok.board, rdat->w / 2,
                                       rdat->h / 2, smaller * 0.9);
    rdat->rboard = &d->rboard;
  }

  size_t pid = crok_spawn_player(
      &d->crok, crok_player_init(CROK_U_TEAM1 | CROK_U_P1, "local player"));
  crok_spawn_disc(&d->crok, &d->crok.players[pid]);
  d->crok.players[pid].pos.x = CROK_BOARD_DIAM / 2;
  d->crok.players[pid].pos.y = CROK_BOARD_DIAM / 2;

  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "setup finished"};
}

struct sched_trans crok_cleanup(struct scheduler *s) {
  struct crok_data *d = (struct crok_data *)s->persistent_data;
  struct crok_setup_data *setdat = (struct crok_setup_data *)s->setup_data;

  if (!setdat->servermode) {
    struct crok_render_data *r = &d->renderdata;
    crok_board_render_finish(d->renderdata.rboard);
    SDL_DestroyRenderer(r->rdr);
    SDL_DestroyWindow(r->win);
    SDL_Quit();
  }

  d = NULL;
  free(s->persistent_data);
  s->persistent_data = NULL;

  return (struct sched_trans){.s = SCHED_HALT, .why = "cleanup finished"};
}

struct sched_trans crok_fixed(struct scheduler *s, long step_ns) {
  struct crok_data *d = (struct crok_data *)s->persistent_data;
  struct crok_state *cs = (struct crok_state *)&d->crok;
  static long num_ticks = 0;
  num_ticks += 1;
  if (num_ticks * step_ns >= 10 * (long)1e9) {
    return (struct sched_trans){.s = SCHED_CLEANUP,
                                .why = "60 seconds elapsed"};
  }

  struct crok_player *p = &cs->players[cs->active_player];
  if (!crok_unit_valid(p)) {
    cs->gamestage = CROK_ERROR;
    return (struct sched_trans){.s = SCHED_ERR, .why = "invalid active player"};
  }
  // Side effect-y, mutates this clients players
  enum crok_gamestage gst = crok_process_input(d);
  switch (gst) {
  case CROK_LEAVE:
    return (struct sched_trans){.s = SCHED_CLEANUP,
                                .why = "user requested quit"};
    break;
  default:
    break;
  }

  struct crok_disc *td = cs->discs;
  struct crok_player *tp = cs->players;

  if (!d->renderdata.servermode) {
    struct ivec target = tp->pos;
    struct ivec force = ivec_sub(target, td->pb.pos);
    physbody_force(&td->pb, ivec_mul(td->pb.vel, (struct ivec){-1, -1}),
                   CROK_TPS);
    physbody_force(&td->pb, force, CROK_TPS);
  }

  for (size_t i = 0; i < CROK_DISCS_TOTAL; i++) {
    struct crok_disc *disc = &d->crok.discs[i];
    if (crok_unit_valid(disc)) {
      crok_disc_tick(disc, &d->crok);
    }
  }

  const unsigned char *state_ser = (unsigned char *)crok_state_serialize(cs);
  uint32_t chksum = crc_32(state_ser, CROK_STATE_SERLEN);
  printf("tick %ld chk %u\n", num_ticks, chksum);

  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "fixed call"};
}

void crok_render(struct scheduler *s, double a) {
  struct crok_data *d = (struct crok_data *)s->persistent_data;
  struct crok_render_data *r = &d->renderdata;

  SDL_SetRenderDrawColor(r->rdr, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(r->rdr);

  SDL_RenderCopy(r->rdr, d->rboard.tex, NULL, &d->rboard.dest);
  for (size_t i = 0; i < 4; i++) {
    struct crok_player *player[2] = {&d->crok.players[i], NULL};
    if (crok_unit_valid(player[0])) {
      crok_player_render(player, r);
    }
  }

  for (size_t i = 0; i < CROK_DISCS_TOTAL; i++) {
    struct crok_disc *disc[2] = {&d->crok.discs[i], NULL};
    if (crok_unit_valid(disc[0])) {
      crok_disc_render(disc, r);
    }
  }

  // triggers the double buffers for multiple rendering
  SDL_RenderPresent(r->rdr);
}

enum crok_gamestage crok_process_input(struct crok_data *d) {
  struct crok_state *st = &d->crok;
  struct crok_input_context *inpctx = &d->input;
  struct crok_render_data *rdr = &d->renderdata;
  struct crok_player *driven_player = &st->players[d->my_player_id];
  enum crok_gamestage new_gst = st->gamestage;
  struct fvec spos;

  crok_input_accum(inpctx, crok_local_input, NULL);
  for (size_t i = 0; i <= inpctx->buf_last; i++) {
    struct crok_input_packet pckt = inpctx->buf[i];
    switch (pckt.kind) {
    case CROK_INPUT_INVALID:
      // FIXME: Should this fail?
      break;
    case CROK_INPUT_POS:
      spos = (struct fvec){.x = pckt.pos.pos.x, .y = pckt.pos.pos.y};
      driven_player->pos = crok_screen_to_board(rdr, spos);
      break;
    case CROK_INPUT_LAY_DISC:
      break;
    case CROK_INPUT_AIM_DISC:
      break;
    case CROK_INPUT_RECALL_DISC:
      break;
    case CROK_INPUT_SHOOT_DISC:
      break;
    case CROK_INPUT_READY:
      break;
    case CROK_INPUT_QUIT:
      new_gst = CROK_LEAVE;
      printf("leaving...\n");
      break;
    case CROK_INPUT_END:
      break;
    }
  }
  crok_input_clean(inpctx);
  return new_gst;
}

size_t crok_spawn_player(struct crok_state *cs, struct crok_player p) {
  size_t i = crok_unit_vacantpos(cs->players, sizeof(struct crok_player), 4);
  if (i >= 0) {
    cs->players[i] = p;
  }
  return i;
}

size_t crok_spawn_disc(struct crok_state *cs, struct crok_player *owner) {
  size_t i = crok_unit_vacantpos(cs->discs, sizeof(struct crok_disc),
                                 CROK_DISCS_TOTAL);
  if (i >= 0) {
    cs->discs[i] = crok_disc_init(owner);
  }
  return i;
}
