#include "crok.h"

struct crok_net_pcktrecord crok_network_input(void *data) {
  struct crok_net_context *nc = data;
  struct crok_net_pcktrecord r;
  r.origin = crok_net_rx(nc);
  r.pckt = nc->rx;
  return r;
}

struct sched_trans crok_setup(struct scheduler *s) {
  struct crok_setup_data *setdat = (struct crok_setup_data *)s->setup_data;
  s->persistent_data =
      safe_calloc(sizeof(struct crok_data), "setup persistent_data alloc");
  struct crok_data *d = (struct crok_data *)s->persistent_data;
  *d = crok_game_init(*setdat);

  struct sockaddr_in *saddr = &d->net.saddr;
  ssize_t addrl = sizeof(*saddr);
  memcpy(saddr, &setdat->saddr, addrl);

  d->net.sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (d->net.sockfd == -1) {
    perror("Couldn't create a network socket");
    goto neterr;
  }
  fcntl(d->net.sockfd, F_SETFL, O_NONBLOCK);

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

    // announce our presence
    d->net.tx = (struct crok_net_packet){0};
    d->net.tx.kind = CROK_NET_CONNECT;
    d->net.tx.data_len = pack(d->net.tx.data, 4, MACROSTR(CROK_PLAYER_NAME_MAXLEN) "s", setdat->name);
    crok_net_tx_to_server(&d->net);

    int wait = 0;
    do {
      usleep(5000);
      wait++;
      crok_net_rx(&d->net);
    } while(d->net.rx.kind != CROK_NET_ACK && wait < 50);
    if (wait >= 50) {
      crok_net_print_packet(stdout, &d->net.rx);
      fprintf(stderr, "couldn't log into lobby!\n"); goto neterr; }

    d->my_player_id = d->net.rx.conn_uid;
    if (d->my_player_id == 0xff) { fprintf(stderr, "lobby full. quitting\n"); goto neterr; }
    
        char ip4[INET_ADDRSTRLEN];
    printf("using server %s on port %d...\n",
           inet_ntop(AF_INET, &saddr->sin_addr, ip4, INET_ADDRSTRLEN),
           CROK_PORT);
  }

  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "setup finished"};
neterr:
  crok_net_print_context(stderr, &d->net);
  return (struct sched_trans){.s = SCHED_ERR, .why = "network setup error"};
}

struct sched_trans crok_cleanup(struct scheduler *s) {
  struct crok_data *d = (struct crok_data *)s->persistent_data;
  crok_game_finish(d);
  free(s->persistent_data);
  s->persistent_data = NULL;

  return (struct sched_trans){.s = SCHED_HALT, .why = "cleanup finished"};
}

static void set_ui_stagebits(struct crok_data *d);

struct sched_trans crok_fixed(struct scheduler *s, long step_ns) {
  struct crok_data *d = (struct crok_data *)s->persistent_data;
  struct crok_state *st = &d->crok;
  struct crok_net_context *nc = &d->net;
  struct crok_render_data *rdr = &d->renderdata;
  struct crok_player *my_player = st->players + d->my_player_id;
  d->ticknum += 1;
  /* if (d->ticknum * step_ns >= 10 * (long)1e9) { */
  /*   return (struct sched_trans){.s = SCHED_CLEANUP, */
  /*                               .why = "60 seconds elapsed"}; */
  /* } */

  // twice the size to never overrun with padding
  static pbuf_t stateser[sizeof(struct crok_state)*2];
  size_t len = crok_state_pack(stateser, st);
  d->statesum = crc_32((unsigned char *)stateser, len);

  size_t old_head = nc->circular_head;
  size_t n_new_pckts = crok_net_accum(nc, crok_network_input, nc);
  for (size_t i = 0; i < n_new_pckts; i++) {
    size_t pckt_idx = (old_head + i + 1) % CROK_NETPACKET_MAX;
    struct crok_net_pcktrecord *r = nc->buf + pckt_idx;
    struct crok_net_packet *pckt = &r->pckt;

    if(0 && pckt->kind != CROK_NET_POS) {
      crok_net_print_pcktrecord(stdout, r);
    }
    crok_stage_gameloops[d->crok.stage.kind](d, r);
  }
  
  if (d->servermode) goto end;

  SDL_Event input;
  while (SDL_PollEvent(&input) > 0) {
    switch (input.type) {
    case SDL_QUIT:
      st->stage.kind = CROK_GS_LEAVE;
      printf("leaving...\n");
      nc->tx.kind = CROK_NET_QUIT;
      crok_net_tx_to_server(nc);
      break;
    case SDL_MOUSEMOTION: {
        struct fvec spos = {.x = input.motion.x, .y = input.motion.y};
        struct ivec bpos = crok_screen_to_board(rdr, spos);
      	nc->tx.ticknum = d->ticknum;
	      nc->tx.statesum = d->statesum;
	      nc->tx.kind = CROK_NET_POS;
	      nc->tx.conn_uid = d->my_player_id;
      	nc->tx.data_len = ivec_pack(nc->tx.data, &bpos);
        crok_net_tx_to_server(nc);
      }
      break;
    case SDL_MOUSEBUTTONUP:
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (st->stage.kind == CROK_GS_ROUND) {
        if (st->stage.data.active_player != d->my_player_id) {
          nc->tx.kind = CROK_NET_LAY_DISC;
          nc->tx.conn_uid = d->my_player_id;
          nc->tx.data_len = 0;
          crok_net_tx_to_server(nc);
        }
      }
      break;
    default:
      break;
    }
  }
  
  set_ui_stagebits(d);
  end:
  switch (st->stage.kind) {
    case CROK_GS_LOBBY: break;
    case CROK_GS_SIDESEL: break;
    case CROK_GS_ROUND: break;
    case CROK_GS_LEAVE:
    return (struct sched_trans) {
      .s = SCHED_CLEANUP,
      .why = (d->servermode)? "all players left" : "user requested quit"
    };
    break;
    case CROK_GS_ERROR:
    return (struct sched_trans) {.s = SCHED_ERR, .why = d->crok.stage.data.why};
    break;
    case CROK_GS_END: break;
  }

  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "fixed call"};
}

void crok_render(struct scheduler *s, double a) {
  struct crok_data *d = (struct crok_data *)s->persistent_data;
  struct crok_render_data *r = &d->renderdata;

  SDL_SetRenderDrawColor(r->rdr, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(r->rdr);

  SDL_RenderCopy(r->rdr, d->renderdata.board.tex, NULL, &d->renderdata.board.dest);
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

  crok_ui_render(d);

  // triggers the double buffers for multiple rendering
  SDL_RenderPresent(r->rdr);
}

static void
set_ui_stagebits(struct crok_data *d) {
  static enum crok_gstage_kind lastk = CROK_GS_END;
  if (lastk != d->crok.stage.kind) {
    lastk = d->crok.stage.kind;

    crok_ui_free_text(&d->renderdata.ui.gamestage);
    d->renderdata.ui.gamestage = crok_ui_new_text(
      d->renderdata.rdr, &d->renderdata.ui,
      (char *)crok_gstage_kind_name[d->crok.stage.kind]
    );
  }
}
