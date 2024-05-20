#include "crok.h"
#include "crok/data.h"

size_t crok_gstage_serde(pbuf_t *head, char mode, void* io) {
  struct crok_gstage *iot = io;
  size_t align = alignof(struct crok_gstage);
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(struct crok_gstage);
  switch (iot->kind) {
    case CROK_GS_LOBBY:
    case CROK_GS_SIDESEL:
      return pack_unpack(head, mode, align, "C", &iot->data.players_ready);
    case CROK_GS_ROUND:
      return pack_unpack(head, mode, align, "C", &iot->data.active_player);
    default:
      return 0;
  }
}

size_t ivec_serde(pbuf_t *buf, char mode, void *vp) {
  struct ivec *v = vp;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(struct ivec);
  return pack_unpack(buf, mode, alignof(struct ivec), "QQ", &v->x, &v->y);
}

size_t fvec_serde(pbuf_t *buf, char mode, void *vp) {
  struct fvec *v = vp;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(struct fvec);
  return pack_unpack(buf, mode, alignof(struct fvec), "II", &v->x, &v->y);
}

size_t physbody_serde(pbuf_t *buf, char mode, void *pbp) {
  struct physbody *pb = pbp;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(struct physbody);
  return pack_unpack(buf, mode, alignof(struct physbody),
          "vvv",
          &pb->pos, &pb->vel, &pb->acc);
}

size_t crok_player_serde(pbuf_t *buf, char mode, void *pp) {
  struct crok_player *p = pp;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(struct crok_player);
  return pack_unpack(buf, mode, alignof(struct crok_player), "vIH" MACROSTR(CROK_PLAYER_NAME_MAXLEN) "s",
          &p->pos, &p->score, &p->flags, &p->name);
}

size_t crok_disc_serde(pbuf_t *dst, char mode, void *dp) {
  struct crok_disc *d = dp;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(struct crok_disc);
  return pack_unpack(dst, mode, alignof(struct crok_disc), "pI", &d->pb, &d->flags);
}

size_t crok_state_serde(pbuf_t *dst, char mode, void *stp) {
  struct crok_state *st = stp;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(struct crok_state);
  return pack_unpack(dst, mode, alignof(struct crok_state), "4P" MACROSTR(CROK_DISCS_TOTAL) "DIIC[C]",
              &st->players, &st->discs, &st->round,
              &st->overall_score, &st->total_players,
              &crok_gstage_serde, &st->stage);
}

static void
serde_init() {
  serde_fun *t = serializer_table;
  t['D'] = crok_disc_serde;
  t['p'] = physbody_serde;
  t['v'] = fvec_serde;
  t['P'] = crok_player_serde;
  t['G'] = crok_state_serde;
}

struct crok_data crok_setup(struct crok_setup_data *setdat) {
  struct crok_data d = crok_game_init(setdat);
  netmgr *net = &d.net;
  pbuf_t *netdata;
  crok_net_preamble preamble = {0};
  serde_init();

  struct sockaddr_in *saddr = &d.srv.server_a;
  ssize_t addrl = sizeof(*saddr);
  memcpy(saddr, &setdat->saddr, addrl);

  net->sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (net->sock_fd == -1) {
    perror("Couldn't create a network socket");
    goto neterr;
  }
  fcntl(d.net.sock_fd, F_SETFL, O_NONBLOCK);
  net->spkinds.onblock = CROK_NET_END;
  net->spkinds.onerror = CROK_NET_INVALID;

  if (d.servermode) { // server
    saddr->sin_addr.s_addr = INADDR_ANY;

    int retc = bind(d.net.sock_fd, (struct sockaddr *)saddr, addrl);
    if (retc < 0) {
      perror("Couldn't bind network socket");
      goto neterr;
    }
    
    printf("server running on port %d...\n", CROK_PORT);
  } else { // client

    if (connect(d.net.sock_fd, (struct sockaddr *)saddr, addrl) < 0) {
      perror("Couldn't connect to server");
      goto neterr;
    }

    // announce our presence
    net->peer = d.srv.server_a;
    net->tx = (netpacket){0};
    net->tx.kind = CROK_NET_CONNECT;
    netdata = crok_net_preamble_put(&net->tx, &preamble);
    net->tx.data_len += pack(netdata, 4, MACROSTR(CROK_PLAYER_NAME_MAXLEN) "s", setdat->name);
    netmgr_send(net);

    int wait = 0;
    do {
      usleep(500000);
      wait++;
      netmgr_recv(net);
    } while(net->rx.kind != CROK_NET_ACK && wait < 50);
    if (wait >= 50) {
      char *packetstr = netpacket_owned_debug_str(&net->rx);
      fprintf(stderr, "couldn't log into lobby! got packet:\n%s\n", packetstr);
      free(packetstr);
      goto neterr;
    }

    netdata = crok_net_preamble_take(&net->rx, &preamble);

    d.my_player_id = preamble.conn_uid;
    if (d.my_player_id == 0xff) { fprintf(stderr, "lobby full. quitting\n"); goto neterr; }
    
        char ip4[INET_ADDRSTRLEN];
    printf("using server %s on port %d...\n",
           inet_ntop(AF_INET, &saddr->sin_addr, ip4, INET_ADDRSTRLEN),
           CROK_PORT);
  }

  return d;
neterr:
  d.crok.stage.kind = CROK_GS_ERROR;
  d.error_description = "network setup error";
  return d;
}

void crok_cleanup(struct crok_data *d) {
  crok_game_finish(d);
}

static void set_ui_stagebits(struct crok_data *d);
static void user_input(struct crok_data *d, struct fvec *newpos) {
  netmgr *net = &d->net;
  SDL_Event input;
  while (SDL_PollEvent(&input) > 0) {
    switch (input.type) {
    case SDL_QUIT:
      d->crok.stage.kind = CROK_GS_LEAVE;
      printf("leaving...\n");
      net->tx.kind = CROK_NET_QUIT;
      netmgr_send(net);
      break;
    case SDL_MOUSEMOTION: {
        struct fvec spos = {.x = input.motion.x, .y = input.motion.y};
        //struct fvec bposf = crok_screen_to_board(rdr, spos);
        //struct ivec bpos = {.x = bposf.x, .y=bposf.y};
        *newpos = spos;
      }
      break;
    case SDL_MOUSEBUTTONUP:
      break;
    case SDL_MOUSEBUTTONDOWN:
      net->tx.kind = CROK_NET_CLICK;
      //net->tx.ticknum = d->ticknum;
      //net->tx.statesum = d->statesum;
      //net->tx.conn_uid = d->my_player_id;
      net->tx.data_len = 0;
      netmgr_send(net);
      break;
    case SDL_KEYDOWN:
      switch (input.key.keysym.sym) {
        case SDLK_RETURN:
        case SDLK_RETURN2:
        case SDLK_KP_ENTER:
          net->tx.kind = CROK_NET_READY;
          //net->tx.conn_uid = d->my_player_id;
          //net->tx.ticknum = d->ticknum;
          //net->tx.statesum = d->statesum;
          net->tx.data_len = 0;
          netmgr_send(net);
          break;
        case SDLK_BACKSPACE:
          net->tx.kind = CROK_NET_CANCEL;
          // net->tx.conn_uid = d->my_player_id;
          // net->tx.ticknum = d->ticknum;
          // net->tx.statesum = d->statesum;
          net->tx.data_len = 0;
          break;
        case SDLK_h:
          d->should_reload = 1;
      }
    default:
      break;
    }
  }
}

int crok_fixed(struct crok_data *d, long step_ns) {
  struct crok_state *st = &d->crok;
  netmgr *net = &d->net;
  pbuf_t *netdata;
  crok_net_preamble preamble;
  struct crok_render_data *rdr = &d->renderdata;
  struct crok_player *my_player = st->players + d->my_player_id;

  d->ticknum += 1;

  // twice the size to never overrun with padding
  static pbuf_t stateser[sizeof(struct crok_state)*2];
  size_t len = pack(stateser, alignof(struct crok_state), "G", st);
  d->statesum = crc_32((unsigned char *)stateser, len);

  for (size_t i = 0; i < 10000; i++) {
    const char* reject_reason;
    netmgr_sprecv(net);
    netpacket *pckt = &net->rx;

    //if (pckt->conn_uid >= 4) {
    //  reject_reason = "connection unique id out of bounds for packet:";
      // goto reject_packet;
    // }
    if (pckt->kind == CROK_NET_END) break;
    if (pckt->kind > CROK_PACKET_MAX_DATA_SIZE) {
      reject_reason = "packet too large:";
      goto reject_packet;
    }
    
    crok_stage_gameloops[d->crok.stage.kind](d, pckt);

    continue;
    reject_packet:
      fputs(reject_reason, stderr);
  }

  struct fvec newpos = my_player->pos;
  if (!d->servermode) user_input(d, &newpos);
  if (newpos.x != my_player->pos.x || newpos.y != my_player->pos.y) {
    //preamble.ticknum  = 0xddeeaaddbbeeeeff;
    //preamble.statesum = 0xdeadbeef;
    preamble.ticknum  = d->ticknum;
    preamble.statesum = d->statesum;
    preamble.conn_uid = d->my_player_id;
    net->tx.kind = CROK_NET_POS;
    net->tx.data_len = 0;
    netdata = crok_net_preamble_put(&net->tx, &preamble);
    net->tx.data_len += pack(netdata, alignof(struct fvec), "v", &newpos);
    char *pstr = netpacket_owned_debug_str(&net->tx);
    printf("sending to srv %s\n", pstr);
    free(pstr);
    netmgr_send(net);
  }
  
  if (!d->servermode) set_ui_stagebits(d);
  end:
  switch (st->stage.kind) {
    case CROK_GS_LOBBY: break;
    case CROK_GS_SIDESEL: break;
    case CROK_GS_ROUND: break;
    case CROK_GS_LEAVE:
      return 0; // all players left / user requested quit
      break;
    case CROK_GS_ERROR:
      d->error_description = (char*)d->crok.stage.data.why;
      return 0;
    break;
    case CROK_GS_END: break;
  }

  return 1;
}

void crok_render(struct crok_data *d, double a) {
  struct crok_render_data *r = &d->renderdata;

  SDL_SetRenderDrawColor(r->rdr, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(r->rdr);

  SDL_RenderCopy(r->rdr, r->board.tex, NULL, &r->board.dest);
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
