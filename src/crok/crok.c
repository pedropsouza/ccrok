#include "crok.h"

struct crok_net_packet crok_local_input(struct crok_net_context *ic) {

  SDL_Event input;
  struct crok_net_packet ipckt = {0};
  ipckt.header.ticknum = ic->ticknum;
  ipckt.header.statesum = ic->state_chksum;

  if (SDL_PollEvent(&input) > 0) {
    switch (input.type) {
    case SDL_QUIT:
      ipckt.header.kind = CROK_NET_QUIT;
      break;
    case SDL_MOUSEMOTION:
      ipckt.header.kind = CROK_NET_POS;
      ipckt.pos = (struct ivec){.x = input.motion.x, .y = input.motion.y};
      break;
    case SDL_MOUSEBUTTONUP:
      break;
    case SDL_MOUSEBUTTONDOWN:
      break;
    default:
      break;
    }
  } else {
    ipckt.header.kind = CROK_NET_END;
  }
  return ipckt;
}

struct crok_net_packet crok_network_input(struct crok_net_context *ic) {}

struct sched_trans crok_setup(struct scheduler *s) {
  s->persistent_data =
      safe_calloc(sizeof(struct crok_data), "setup persistent_data alloc");
  struct crok_data *d = (struct crok_data *)s->persistent_data;
  struct crok_setup_data *setdat = (struct crok_setup_data *)s->setup_data;

  d->width = setdat->width;
  d->height = setdat->height;
  d->servermode = setdat->servermode;
  d->crok = crok_state_init();

  struct sockaddr_in *saddr = &d->net.saddr;
  ssize_t addrl = sizeof(*saddr);
  memcpy(saddr, &setdat->saddr, addrl);

  d->net.sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (d->net.sockfd == -1) {
    perror("Couldn't create a network socket");
    goto neterr;
  }
  fcntl(d->net.sockfd, F_SETFL, O_NONBLOCK);

  // setup the network sockets first, if that fails
  // the cleanup is easier
  if (d->servermode) { // server
    saddr->sin_addr.s_addr = INADDR_ANY;

    int retc = bind(d->net.sockfd, (struct sockaddr *)saddr, addrl);
    if (retc < 0) {
      perror("Couldn't bind network socket");
      goto neterr;
    }

    printf("server running on port %d...\n", CROK_PORT);
  } else { // client

    if (connect(d->net.sockfd, (struct sockaddr *)saddr, addrl) < 0) {
      perror("Couldn't connect to server");
      goto neterr;
    }

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

    char ip4[INET_ADDRSTRLEN];
    printf("using server %s on port %d...\n",
           inet_ntop(AF_INET, &saddr->sin_addr, ip4, INET_ADDRSTRLEN),
           CROK_PORT);

    int tpid = crok_spawn_player(
        &d->crok, crok_player_init(CROK_U_TEAM1 | CROK_U_P1, "test player"));
    d->crok.active_player = tpid;
    crok_spawn_disc(&d->crok, &d->crok.players[tpid]);
  }

  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "setup finished"};
neterr:
  return (struct sched_trans){.s = SCHED_ERR, .why = "network setup error"};
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
  d->ticknum += 1;
  if (d->ticknum * step_ns >= 10 * (long)1e9) {
    return (struct sched_trans){.s = SCHED_CLEANUP,
                                .why = "60 seconds elapsed"};
  }

  if (cs->total_players > 0) {
    struct crok_player *p = &cs->players[cs->active_player];
    if (!crok_unit_valid(p)) {
      cs->gamestage = CROK_ERROR;
      return (struct sched_trans){.s = SCHED_ERR,
                                  .why = "invalid active player"};
    }
  }

  if (!d->servermode) {
    // Side effect-y, mutates this clients player
    enum crok_gamestage gst = crok_process_client(d);
    switch (gst) {
    case CROK_LEAVE:
      return (struct sched_trans){.s = SCHED_CLEANUP,
                                  .why = "user requested quit"};
      break;
    default:
      break;
    }

    struct crok_player *tp = &cs->players[cs->active_player];
    struct crok_disc *td = &cs->discs[0];
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

enum crok_gamestage crok_process_client(struct crok_data *d) {
  struct crok_state *st = &d->crok;
  struct crok_net_context *inpctx = &d->net;
  struct crok_render_data *rdr = &d->renderdata;
  struct crok_player *driven_player = &st->players[d->my_player_id];
  enum crok_gamestage new_gst = st->gamestage;
  struct fvec spos;

  pbuf_t stateser[sizeof(struct crok_state)];
  size_t len = crok_state_pack(stateser, st);

  crok_net_tickdata(inpctx, d->ticknum, crc_32((unsigned char *)stateser, len));

  crok_net_accum(inpctx, crok_local_input);
  for (size_t i = 0; i <= inpctx->buf_last; i++) {
    struct crok_net_packet pckt = inpctx->buf[i];
    switch (pckt.header.kind) {
    case CROK_NET_INVALID:
      // FIXME: Should this fail?
      break;

    case CROK_NET_CONNECT:
      break;
    case CROK_NET_READY:
      break;
    case CROK_NET_QUIT:
      new_gst = CROK_LEAVE;
      printf("leaving...\n");
      break;

    case CROK_NET_POS:
      spos = (struct fvec){.x = pckt.pos.x, .y = pckt.pos.y};
      driven_player->pos = crok_screen_to_board(rdr, spos);
      break;
    case CROK_NET_LAY_DISC:
      break;
    case CROK_NET_AIM_DISC:
      break;
    case CROK_NET_RECALL_DISC:
      break;
    case CROK_NET_SHOOT_DISC:
      break;

    case CROK_NET_STATEDUMP:
      break;
    case CROK_NET_ACK:
      break;
    case CROK_NET_DESYNC:
      break;

    case CROK_NET_END:
      break;
    }
  }
  crok_net_clean(inpctx);
  return new_gst;
}

enum crok_gamestage crok_process_server(struct crok_data *d) {
  struct crok_state *st = &d->crok;
  struct crok_net_context *inpctx = &d->net;
  struct crok_render_data *rdr = &d->renderdata;
  struct crok_player *driven_player = &st->players[d->my_player_id];
  enum crok_gamestage new_gst = st->gamestage;
  struct fvec spos;

  pbuf_t stateser[sizeof(struct crok_state)];
  size_t len = crok_state_pack(stateser, st);

  crok_net_tickdata(inpctx, d->ticknum, crc_32((unsigned char *)stateser, len));

  crok_net_accum(inpctx, crok_local_input);
  for (size_t i = 0; i <= inpctx->buf_last; i++) {
    struct crok_net_packet pckt = inpctx->buf[i];
    switch (pckt.header.kind) {
    case CROK_NET_INVALID:
      // FIXME: Should this fail?
      break;

    case CROK_NET_CONNECT:
      break;
    case CROK_NET_READY:
      break;
    case CROK_NET_QUIT:
      new_gst = CROK_LEAVE;
      printf("leaving...\n");
      break;

    case CROK_NET_POS:
      spos = (struct fvec){.x = pckt.pos.x, .y = pckt.pos.y};
      driven_player->pos = crok_screen_to_board(rdr, spos);
      break;
    case CROK_NET_LAY_DISC:
      break;
    case CROK_NET_AIM_DISC:
      break;
    case CROK_NET_RECALL_DISC:
      break;
    case CROK_NET_SHOOT_DISC:
      break;

    case CROK_NET_STATEDUMP:
      break;
    case CROK_NET_ACK:
      break;
    case CROK_NET_DESYNC:
      break;

    case CROK_NET_END:
      break;
    }
  }
  crok_net_clean(inpctx);
  return new_gst;
}

ssize_t crok_spawn_player(struct crok_state *cs, struct crok_player p) {
  ssize_t i = crok_unit_vacantpos(cs->players, sizeof(struct crok_player), 4);
  if (i >= 0) {
    cs->players[i] = p;
  }
  return i;
}

ssize_t crok_spawn_disc(struct crok_state *cs, struct crok_player *owner) {
  ssize_t i = crok_unit_vacantpos(cs->discs, sizeof(struct crok_disc),
                                  CROK_DISCS_TOTAL);
  if (i >= 0) {
    cs->discs[i] = crok_disc_init(owner);
  }
  return i;
}
