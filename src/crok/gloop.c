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
  struct fvec spos = crok_board_to_screen(rd, p[0]->pos);
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
  d.pb.radius = CROK_DISC_DIAM;
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
  struct fvec spos = crok_board_to_screen(rd, d[0]->pb.pos);
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

static void
universal(struct crok_data *d, struct crok_net_pcktrecord *pcktr) {
  struct crok_net_packet *pckt = &pcktr->pckt;
  switch (pckt->kind) {
    case CROK_NET_POS: {
      struct ivec recvd_pos;
      ivec_unpack(pckt->data, &recvd_pos);
      d->crok.players[pckt->conn_uid].pos = recvd_pos;

      if (d->servermode) {
        d->net.tx = (struct crok_net_packet){
          .ticknum = d->ticknum,
  	      .statesum = d->statesum,
  	      .kind = CROK_NET_POS,
  	      .conn_uid = pckt->conn_uid
        };
        d->net.tx.data_len = ivec_pack(d->net.tx.data, &recvd_pos);
        crok_net_tx_to_game(&d->net);
      }
      break;
    }
    case CROK_NET_PLAYER_FLAGS:
      unpack(pckt->data, 1, "C", &d->crok.players[pckt->conn_uid].flags);
      break;
    case CROK_NET_STATEDUMP:
      if (!d->servermode) {
        crok_state_unpack(pckt->data, &d->crok);
        crok_state_print(&d->crok);
        d->ticknum = pckt->ticknum;
        update_nameplates(d);
      }
      break;
    default:
      fprintf(stderr, "Unhandled packet:\n");
      crok_net_print_pcktrecord(stderr, pcktr);
      break;
  }
}

static void
lobby(struct crok_data *d, struct crok_net_pcktrecord *pcktr) {
  struct crok_net_packet *pckt = &pcktr->pckt;
  switch (pckt->kind) {
    case CROK_NET_CONNECT:
      if (d->servermode) {
        char flags = 0;
        int uid = -1;
        struct crok_player newp = crok_player_init(flags, "");
        unpack(pckt->data, 4, "s", newp.name);
        if (strnlen((char *)newp.name, CROK_PLAYER_NAME_MAXLEN) == 0) {
          strcpy(newp.name, "anon");
        }
        newp.pos.x = d->board.arena_units/2;
        newp.pos.y = d->board.arena_units/2;
        uid = crok_spawn_player(&d->crok, newp);
        d->net.tx.kind = CROK_NET_ACK;
        d->net.tx.conn_uid = (uid < 0)? 0xff : uid;

        crok_net_tx_to_addr(&d->net, &pcktr->origin);

        if (uid < 0) break; // already full
        d->crok.total_players++;

        d->net.players[uid] = pcktr->origin;

        d->net.tx.kind = CROK_NET_STATEDUMP;
        d->net.tx.data_len = crok_state_pack((pbuf_t*)&d->net.tx.data, &d->crok);
        d->net.tx.ticknum = d->ticknum;
        d->net.tx.statesum = d->statesum;
        crok_net_tx_to_game(&d->net);

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
        d->net.tx = *pckt;
        crok_net_tx_to_game(&d->net);
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

      d->net.tx.kind = CROK_NET_STATEDUMP;
      d->net.tx.conn_uid = 0;
      d->net.tx.data_len = crok_state_pack(d->net.tx.data, &d->crok);
      d->net.tx.ticknum = d->ticknum;
      d->net.tx.statesum = d->statesum;
      crok_net_tx_to_game(&d->net);
      break;
    
    default:
      universal(d, pcktr);
      break;
  }
}

// algo used to let players select their team
static uint8_t side2team(uint64_t x) {
  return (x < UINT64_MAX/2) ? CROK_U_TEAM1 : CROK_U_TEAM2;
}

static void
sidesel(struct crok_data *d, struct crok_net_pcktrecord *pcktr) {
  struct crok_net_packet *pckt = &pcktr->pckt;
  if (!d->servermode) goto end;
  if (pckt->kind == CROK_NET_POS || pckt->kind == CROK_NET_CLICK) {
    struct crok_player *p = d->crok.players + pckt->conn_uid;
    if ((1 << pckt->conn_uid) & d->crok.stage.data.players_ready) goto end;
    if (!crok_unit_valid(p)) {
      fprintf(stderr, "invalid player tried to select a team:");
      crok_player_print(p);
      return;
    }
    struct ivec pos; ivec_unpack(pckt->data, &pos);
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

    d->net.tx.kind = CROK_NET_PLAYER_FLAGS;
    d->net.tx.conn_uid = pckt->conn_uid;
    d->net.tx.data_len = pack(d->net.tx.data, 1, "C", &p->flags);
    d->net.tx.ticknum = d->ticknum;
    d->net.tx.statesum = d->statesum;
    crok_net_tx_to_game(&d->net);

    if (pckt->kind == CROK_NET_CLICK) {
      d->crok.stage.data.players_ready |= 1 << pckt->conn_uid;
      if (d->crok.stage.data.players_ready == 0b1111) {
        d->crok.stage.kind = CROK_GS_ROUND;
        d->crok.stage.data.active_player = 0;
      }
      
      d->net.tx.kind = CROK_NET_STATEDUMP;
      d->net.tx.conn_uid = 0;
      d->net.tx.data_len = crok_state_pack(d->net.tx.data, &d->crok);
      d->net.tx.ticknum = d->ticknum;
      d->net.tx.statesum = d->statesum;
      crok_net_tx_to_game(&d->net);
    }
  }
  if (pckt->kind == CROK_NET_CANCEL) {
    d->crok.stage.data.players_ready &= ~(1 << pckt->conn_uid);
  }
  end: universal(d, pcktr);
}
static void
gameround(struct crok_data *d, struct crok_net_pcktrecord *pcktr) {
  if (d->crok.total_players > 0) {
    struct crok_player *p = &d->crok.players[d->crok.stage.data.active_player];
    if (!crok_unit_valid(p)) {
      d->crok.stage.kind = CROK_GS_ERROR;
      d->crok.stage.data.why = "invalid active player";
    }
  }
  universal(d, pcktr);
}
static void
leave(struct crok_data *d, struct crok_net_pcktrecord *pcktr) {
  universal(d, pcktr);
}
static void
error(struct crok_data *d, struct crok_net_pcktrecord *pcktr) {
  universal(d, pcktr);
}

crok_stage_gameloop_func crok_stage_gameloops[CROK_GS_END] = {
  [CROK_GS_LOBBY] = lobby,
  [CROK_GS_SIDESEL] = sidesel,
  [CROK_GS_ROUND] = gameround,
  [CROK_GS_LEAVE] = leave,
  [CROK_GS_ERROR] = error,
};

