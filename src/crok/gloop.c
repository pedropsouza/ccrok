#include "crok/gloop.h"

struct crok_player
crok_player_init(uint16_t flags, char *name) {
  struct crok_player p = {0};
  strncpy(p.name, name, CROK_PLAYER_NAME_MAXLEN - 1);
  p.name[CROK_PLAYER_NAME_MAXLEN - 1] = '\0';
  p.flags = CROK_U_VALID | flags;
  p.score = 0;
  return p;
}

void
crok_player_render(struct crok_player *p[2], struct crok_render_data *rd) {
  struct fvec spos = p[0]->pos;//crok_board_to_screen(rd, (struct fvec){.x = p[0]->pos.x, .y = p[0]->pos.y});
  if (p[0]->flags & CROK_U_TEAM1) {
    SDL_SetRenderDrawColor(rd->rdr, 0x22, 0x77, 0xf7, 0xff);
  } else {
    SDL_SetRenderDrawColor(rd->rdr, 0xf7, 0xb7, 0x22, 0xff);
  }
  SDL_RenderDrawCircle(rd->rdr, spos.x, spos.y, rd->w / 200);
}

struct crok_disc
crok_disc_init(struct crok_player *owner) {
  struct crok_disc d = {.pb = physbody_zero("disc"), .flags = owner->flags};
  d.pb.pos.x = CROK_BOARD_DIAM / 2;
  d.pb.pos.y = CROK_BOARD_DIAM / 2;
  //d.pb.radius = CROK_DISC_DIAM;
  return d;
}

void
crok_disc_tick(struct crok_disc *d, struct crok_state *s) {
  physbody_tick(&d->pb);
}

void
crok_disc_render(struct crok_disc *d[2], struct crok_render_data *rd) {
  SDL_Rect dest = {0};
  double s = rd->board.disc_diam;
  struct fvec spos = d[0]->pb.pos;//crok_board_to_screen(rd, d[0]->pb.pos);
  dest.w = s;
  dest.h = s;
  dest.x = spos.x - (s - 1.0) / 2.0;
  dest.y = spos.y - (s - 1.0) / 2.0;

  SDL_RenderCopy(rd->rdr, rd->board.disc_tex, NULL, &dest);
}

ssize_t
crok_spawn_player(struct crok_state *cs, struct crok_player p) {
  ssize_t i = crok_unit_vacantpos(cs->players, sizeof(struct crok_player), 4);
  if (i >= 0) {
    cs->players[i] = p;
  }
  return i;
}

ssize_t
crok_spawn_disc(struct crok_state *cs, struct crok_player *owner) {
  ssize_t i = crok_unit_vacantpos(cs->discs, sizeof(struct crok_disc),
                                  CROK_DISCS_TOTAL);
  if (i >= 0) {
    cs->discs[i] = crok_disc_init(owner);
  }
  return i;
}

static void
update_nameplates(struct crok_data *d) {
  for (int i = 0; i < 4; i++) {
    struct crok_player *p = d->crok.players + i;
    if (crok_unit_valid(p)) {
      struct crok_ui_text *nplate = d->renderdata.ui.nameplates + i;
      crok_ui_free_text(nplate);
      *nplate = crok_ui_new_text(d->renderdata.rdr, &d->renderdata.ui, p->name);
    }
  }
}

// Only call when playing out a round, stage == CROK_GS_ROUND
static void
simulate(struct crok_state *cs) {
  assert(cs->stage.kind == CROK_GS_ROUND);
  struct crok_player *tp = &cs->players[cs->stage.data.active_player];
  struct crok_disc *td = &cs->discs[0];
  struct fvec target = (struct fvec){.x = tp->pos.x, .y = tp->pos.y};
  struct fvec force = fvec_sub(target, td->pb.pos);
  physbody_accel(&td->pb, fvec_mul(td->pb.vel, (struct fvec){-10, -10}));
  physbody_accel(&td->pb, force);

  for (size_t i = 0; i < CROK_DISCS_TOTAL; i++) {
    struct crok_disc *disc = &cs->discs[i];
    if (crok_unit_valid(disc)) {
      crok_disc_tick(disc, cs);
    }
  }
}

static void
universal(struct crok_data *d, crok_netpacket *pckt) {
  netmgr *net = &d->net;
  crok_netpacket *tx = (crok_netpacket *)net->tx;
  crok_netpacket *rx = (crok_netpacket *)net->rx; 

  switch (pckt->kind) {
    case CROK_NET_POS: {
      struct fvec recvd_pos;
      unpack(pckt->body_ptr, alignof(struct fvec), "v", &recvd_pos);
      d->crok.players[pckt->conn_uid].pos = recvd_pos;

      if (d->servermode) {
        tx->kind = CROK_NET_POS;
        tx->data_len += pack(tx->body_ptr, alignof(struct fvec), "v", &recvd_pos);
        broadcast(d);
      }
      break;
    }
    case CROK_NET_PLAYER_FLAGS:
      unpack(pckt->body_ptr, 1, "C", &d->crok.players[pckt->conn_uid].flags);
      break;
    case CROK_NET_STATEDUMP:
      if (!d->servermode) {
        unpack(pckt->body_ptr, alignof(struct crok_state), "G", &d->crok);
        crok_state_print(&d->crok);
        d->ticknum = pckt->ticknum;
        update_nameplates(d);
      }
      break;
    default:
      char *pcktstr = netpacket_owned_debug_str((netpacket *)pckt);
      fprintf(stderr, "Unhandled packet:\n%s\n", pcktstr);
      free(pcktstr);
      break;
  }
}

static void
lobby(struct crok_data *d, crok_netpacket *pckt) {
  netmgr *net = &d->net;
  crok_netpacket *tx = (crok_netpacket *)net->tx;
  crok_netpacket *rx = (crok_netpacket *)net->rx; 

  switch (pckt->kind) {
    case CROK_NET_CONNECT:
      if (d->servermode) {
        char *pcktstr = netpacket_owned_debug_str((netpacket *)pckt);
        printf("got connection packet:\n%s\n", pcktstr);
        free(pcktstr);
        char flags = 0;
        int uid = -1;
        struct crok_player newp = crok_player_init(flags, "");

        // WARN: bad alignment has the potential to stack-smash!
        unpack(rx->body_ptr, 8, "s", newp.name);
        if (strnlen((char *)newp.name, CROK_PLAYER_NAME_MAXLEN) == 0) {
          strcpy(newp.name, "anon");
        }
        newp.pos.x = d->board.arena_units/2;
        newp.pos.y = d->board.arena_units/2;
        uid = crok_spawn_player(&d->crok, newp);

        tx->kind = CROK_NET_ACK;
        tx->conn_uid = (uid < 0)? 0xff : uid;
        crok_netpacket_prepare_tx(tx);
        netmgr_send(net);

        if (uid < 0) break; // already full
        d->crok.total_players++;

        d->srv.socks[uid] = net->peer;

        tx->kind = CROK_NET_STATEDUMP;
        tx->ticknum = d->ticknum;
        tx->statesum = d->statesum;
        crok_netpacket_prepare_tx(tx);
        tx->data_len += pack(tx->body_ptr, alignof(struct crok_state), "G", &d->crok);
        char *pstr = netpacket_owned_debug_str((netpacket *)tx);
        printf("broadcasting %s\n", pstr);
        free(pstr);

        broadcast(d);

        printf("Player \"%s\" uid %hhu logged in.\n", d->crok.players[uid].name, (char)uid);
        crok_state_print(&d->crok);
      }
      break;
    case CROK_NET_QUIT:
      d->crok.players[pckt->conn_uid].flags ^= CROK_U_VALID;
      d->crok.total_players--;
      printf("Player \"%s\" uid %hhu left.\n", d->crok.players[pckt->conn_uid].name, pckt->conn_uid);
        
      if (d->servermode) {
        if (!d->crok.total_players) {
          d->crok.stage.kind = CROK_GS_LEAVE;
          printf("leaving...\n");
          //goto end;
        }
        *tx = *pckt;
        broadcast(d);
      } else {
        update_nameplates(d);
      }
      break;
    case CROK_NET_READY:
      d->crok.stage.data.players_ready |= 1 << pckt->conn_uid;
      if (d->crok.stage.data.players_ready == 0b1111) {
        d->crok.stage.kind = CROK_GS_SIDESEL;
        d->crok.stage.data.players_ready = 0;
      }

      tx->kind = CROK_NET_STATEDUMP;
      tx->ticknum = d->ticknum;
      tx->statesum = d->statesum;
      tx->conn_uid = 0;
      crok_netpacket_prepare_tx(tx);
      tx->data_len += pack(tx->body_ptr, alignof(struct crok_state), "G", &d->crok);
      broadcast(d);
      break;
    
    default:
      universal(d, pckt);
      break;
  }
}

// algo used to let players select their team
static uint8_t side2team(int64_t x) {
  return (x < 0) ? CROK_U_TEAM1 : CROK_U_TEAM2;
}

static void
sidesel(struct crok_data *d, crok_netpacket *pckt) {
  netmgr *net = &d->net;
  crok_netpacket *tx = (crok_netpacket *)net->tx;
  crok_netpacket *rx = (crok_netpacket *)net->rx; 

  if (!d->servermode) {
    struct crok_ui_text *dtxt = &d->renderdata.ui.debug;
    crok_ui_free_text(dtxt);
    char buf[1024] = {0};

    struct fvec pos = d->crok.players[d->my_player_id].pos;
    struct fvec spos = crok_board_to_screen(&d->renderdata, pos);
    snprintf(buf, 1024, "b(%li , %li) s(%lf , %lf)", pos.x,pos.y, spos.x, spos.y);
    *dtxt = crok_ui_new_text(d->renderdata.rdr, &d->renderdata.ui, buf);
    dtxt->dst.x = 10;
    dtxt->dst.y = d->renderdata.h - 10 - dtxt->dst.h;
    goto end;
  }
  if (pckt->kind == CROK_NET_POS || pckt->kind == CROK_NET_CLICK) {
    struct crok_player *p = d->crok.players + pckt->conn_uid;
    if ((1 << pckt->conn_uid) & d->crok.stage.data.players_ready) goto end;
    if (!crok_unit_valid(p)) {
      fprintf(stderr, "invalid player tried to select a team:");
      crok_player_print(p);
      return;
    }
    struct ivec pos; unpack(pckt->body_ptr, alignof(struct ivec), "v", &pos);
    char flag = side2team(pos.x);
    char is_second = 0;
    for (int i = 0; i < 4; i++) {
      struct crok_player *p = d->crok.players + i;
      if (crok_unit_valid(p) && (p->flags & flag)) {
        is_second = 1;
        break;
      }
    }
    flag |= is_second ? CROK_U_P2 : CROK_U_P1;

    p->flags = flag | CROK_U_VALID;

    tx->kind = CROK_NET_PLAYER_FLAGS;
    // conn_uid stays the same
    tx->ticknum = d->ticknum;
    tx->statesum = d->statesum;
    crok_netpacket_prepare_tx(tx);
    tx->data_len += pack(tx->body_ptr, 1, "C", &p->flags);
    broadcast(d);

    if (pckt->kind == CROK_NET_CLICK) {
      d->crok.stage.data.players_ready |= 1 << pckt->conn_uid;
      if (d->crok.stage.data.players_ready == 0b1111) {
        d->crok.stage.kind = CROK_GS_ROUND;
        d->crok.stage.data.active_player = 0;
      }
      
      tx->kind = CROK_NET_STATEDUMP;
      tx->ticknum = d->ticknum;
      tx->statesum = d->statesum;
      tx->conn_uid = 0;
      crok_netpacket_prepare_tx(tx);
      tx->data_len += pack(tx->body_ptr, alignof(struct crok_state), "G", &d->crok);
      broadcast(d);
    }
  }
  if (pckt->kind == CROK_NET_CANCEL) {
    d->crok.stage.data.players_ready &= ~(1 << pckt->conn_uid);
  }
  end: universal(d, pckt);
}
static void
gameround(struct crok_data *d, crok_netpacket *pckt) {
  if (d->crok.total_players > 0) {
    struct crok_player *p = &d->crok.players[d->crok.stage.data.active_player];
    if (!crok_unit_valid(p)) {
      d->crok.stage.kind = CROK_GS_ERROR;
      d->crok.stage.data.why = "invalid active player";
    }
  }
  universal(d, pckt);
}
static void
leave(struct crok_data *d, crok_netpacket *pckt) {
  universal(d, pckt);
}
static void
error(struct crok_data *d, crok_netpacket *pckt) {
  universal(d, pckt);
}

crok_stage_gameloop_func crok_stage_gameloops[CROK_GS_END] = {
  [CROK_GS_LOBBY] = lobby,
  [CROK_GS_SIDESEL] = sidesel,
  [CROK_GS_ROUND] = gameround,
  [CROK_GS_LEAVE] = leave,
  [CROK_GS_ERROR] = error,
};

