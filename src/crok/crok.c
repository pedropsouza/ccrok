#include "crok.h"

struct crok_net_pcktrecord crok_network_input(void *data) {
  struct crok_netinput_data *nid = data;
  struct crok_net_pcktrecord r;
  r.origin = crok_net_rx(nid->nc);
  r.pckt = nid->nc->rx;
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
  uint32_t state_chksum = crc_32((unsigned char *)stateser, len);

  struct crok_tick_data td = {
    .tnum = d->ticknum,
    .tsum = state_chksum
  };
  struct crok_netinput_data nid = {
    .nc = nc,
    .td = &td,
  };
  
  size_t old_head = nc->circular_head;
  size_t n_new_pckts = crok_net_accum(nc, crok_network_input, &nid);
  for (size_t i = 0; i < n_new_pckts; i++) {
    size_t pckt_idx = (old_head + i + 1) % CROK_NETPACKET_MAX;
    struct crok_net_pcktrecord *r = nc->buf + pckt_idx;
    struct crok_net_packet *pckt = &r->pckt;

    if(pckt->kind != CROK_NET_POS) {
      crok_net_print_pcktrecord(stdout, r);
    }
    switch (pckt->kind) {

    case CROK_NET_CONNECT:
      if (d->servermode) {
        char flags = 0;
        int uid = -1;
        if (st->gamestage != CROK_LOBBY) {
          goto conn_tx;
        }
        switch (st->total_players) {
            case 0: flags = CROK_U_TEAM1 | CROK_U_P1; break;
            case 1: flags = CROK_U_TEAM2 | CROK_U_P1; break;
            case 2: flags = CROK_U_TEAM1 | CROK_U_P2; break;
            case 3: flags = CROK_U_TEAM2 | CROK_U_P2; break;
        };
        struct crok_player newp = crok_player_init(flags, "");
        unpack(pckt->data, 4, "s", newp.name);
        if (strnlen((char *)newp.name, CROK_PLAYER_NAME_MAXLEN) == 0) {
          strcpy(newp.name, "anon");
        }
        newp.pos.x = d->board.arena_units/2;
        newp.pos.y = d->board.arena_units/2;
        uid = crok_spawn_player(st, newp);
        nc->tx.kind = CROK_NET_ACK;
        nc->tx.conn_uid = (uid < 0)? 0xff : uid;

        conn_tx: crok_net_tx_to_addr(nc, &r->origin);

        if (uid < 0) break; // already full
        st->total_players++;

        nc->players[uid] = r->origin;
        crok_spawn_disc(&d->crok, &d->crok.players[uid]);

        nc->tx.kind = CROK_NET_STATEDUMP;
        nc->tx.data_len = crok_state_pack((pbuf_t*)&nc->tx.data, st);
        nc->tx.statesum = td.tsum;
        nc->tx.ticknum = td.tnum;
        crok_net_tx_to_game(nc);

        printf("Client %s logged in with uid %hhu\n", st->players[uid].name, (char)uid);
        crok_state_print(st);
      }
      break;
    case CROK_NET_QUIT:
      st->players[pckt->conn_uid].flags ^= CROK_U_VALID;
      st->total_players--;
      printf("Player %s uid %hhu left the game.\n", st->players[pckt->conn_uid].name, pckt->conn_uid);
        
      if (d->servermode) {
        if (!st->total_players) {
          st->gamestage = CROK_LEAVE;
          printf("leaving...\n");
          goto end;
        }
        nc->tx = *pckt;
        crok_net_tx_to_game(nc);
      } else {
        crok_update_nameplates(d);
      }
      break;
    case CROK_NET_READY:
      break;
    case CROK_NET_POS: {
      struct ivec recvd_pos;
      ivec_unpack(pckt->data, &recvd_pos);
      st->players[pckt->conn_uid].pos = recvd_pos;

      if (d->servermode) {
        nc->tx = (struct crok_net_packet){
          .ticknum = td.tnum,
  	      .statesum = td.tsum,
  	      .kind = CROK_NET_POS,
  	      .conn_uid = pckt->conn_uid
        };
        nc->tx.data_len = ivec_pack(nc->tx.data, &recvd_pos);
        crok_net_tx_to_game(nc);
      }
      break;
    }
    case CROK_NET_LAY_DISC:
      st->active_player = pckt->conn_uid;
      if (d->servermode) {
        nc->tx = *pckt;
        crok_net_tx_to_game(nc);
      }
      break;
    case CROK_NET_AIM_DISC:
      break;
    case CROK_NET_RECALL_DISC:
      break;
    case CROK_NET_SHOOT_DISC:
      break;

    case CROK_NET_STATEDUMP:
      if (!d->servermode) {
        crok_state_unpack(pckt->data, st);
        crok_state_print(st);
        d->ticknum = pckt->ticknum;
        crok_update_nameplates(d);
      }
      break;
    case CROK_NET_ACK:
      break;
    case CROK_NET_DESYNC:
      break;
    default:
      break;
    }
  }

  if (d->servermode) goto end;

  SDL_Event input;
  while (SDL_PollEvent(&input) > 0) {
    switch (input.type) {
    case SDL_QUIT:
      st->gamestage = CROK_LEAVE;
      printf("leaving...\n");
      nc->tx.kind = CROK_NET_QUIT;
      crok_net_tx_to_server(nc);
      break;
    case SDL_MOUSEMOTION: {
        struct fvec spos = {.x = input.motion.x, .y = input.motion.y};
        struct ivec bpos = crok_screen_to_board(rdr, spos);
      	nc->tx.ticknum = td.tnum;
	      nc->tx.statesum = td.tsum;
	      nc->tx.kind = CROK_NET_POS;
	      nc->tx.conn_uid = d->my_player_id;
      	nc->tx.data_len = ivec_pack(nc->tx.data, &bpos);
        crok_net_tx_to_server(nc);
      }
      break;
    case SDL_MOUSEBUTTONUP:
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (st->active_player != d->my_player_id) {
          nc->tx.kind = CROK_NET_LAY_DISC;
          nc->tx.conn_uid = d->my_player_id;
          nc->tx.data_len = 0;
          crok_net_tx_to_server(nc);
        }
      break;
    default:
      break;
    }
  }
  
  if (st->total_players > 0) {
    struct crok_player *p = &st->players[st->active_player];
    if (!crok_unit_valid(p)) {
      st->gamestage = CROK_ERROR;
      return (struct sched_trans){.s = SCHED_ERR,
                                  .why = "invalid active player"};
    }
  }
  end:
  switch (d->crok.gamestage) {
    case CROK_LOBBY: break;
    case CROK_SIDESEL: break;
    case CROK_ROUNDSTART: break;
    case CROK_AIM: break;
    case CROK_SHOT: break;
    case CROK_VALID: break;
    case CROK_FOUL: break;
    case CROK_TALLY: break;
    case CROK_ROUNDEND: break;
    case CROK_LEAVE:
    return (struct sched_trans) {
      .s = SCHED_CLEANUP,
      .why = (d->servermode)? "all players left" : "user requested quit"
    };
    break;
    case CROK_ERROR: break;
    
  }
  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "fixed call"};
}

void crok_simulate(struct crok_state *cs) {
  struct crok_player *tp = &cs->players[cs->active_player];
  struct crok_disc *td = &cs->discs[0];
  struct ivec target = tp->pos;
  struct ivec force = ivec_sub(target, td->pb.pos);
  physbody_force(&td->pb, ivec_mul(td->pb.vel, (struct ivec){-10, -10}),
                 CROK_TPS);
  physbody_force(&td->pb, force, CROK_TPS);

  for (size_t i = 0; i < CROK_DISCS_TOTAL; i++) {
    struct crok_disc *disc = &cs->discs[i];
    if (crok_unit_valid(disc)) {
      crok_disc_tick(disc, cs);
    }
  }

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
