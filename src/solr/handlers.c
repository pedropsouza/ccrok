#include "solr.h"

void h_join(solr *s, netpacket *p) {
  if (!s->sdata.server) return;
  unsigned char vacant = 0xff;
  for (int i = 0; i < SOLR_MAX_PLAYERS; i++) {
    if (!s->st.players[i].valid) {
      vacant = i;
      break;
    }
  }
  if (vacant == 0xff) {} // TODO

  s->st.players[vacant].valid = 1;
  s->st.players[vacant].pos = (ivec){0};
    
  s->srv.socks[vacant] = s->net.peer;
    
  if (p->data_len > 4 && p->data_len < SOLR_NAME_LEN + 4) {
    unpack(p->data, 4, "s", &s->st.players[vacant].username);
  }

  s->net.tx.kind = PCKT_ACK;
  s->net.tx.data_len = pack(s->net.tx.data, 1, "C", &vacant);
  netmgr_send(&s->net);

  s->net.tx.kind = PCKT_STDUMP;
  s->net.tx.data_len = pack(s->net.tx.data, 8, "G", &s->st);
  broadcast(s);
}

void h_leave(solr *s, netpacket *p) {
  int8_t uid;
  if (!s->sdata.server) return;
  uid = true_uid(s);
  if (uid < 0) return;
  s->srv.socks[uid] = (struct sockaddr_in){0};
  s->st.players[uid].valid = 0;
}

void h_move(solr *s, netpacket *p) {
  card_stack *stacks = s->st.board.stacks;
  move m;
  char *rej_reason;
  unpack(p->data, 1, "m", &m);
  if (!move_valid(s, &m, &rej_reason)) {
    printf("invalid move: %s\n", rej_reason);
    return;
  }

  card_stack *src = stack_from_id(s, m.stacks.src);
  card_stack *dst = stack_from_id(s, m.stacks.dst);

  // TODO: maybe restrict moves coming from players
  // as to not touch other players held stacks?
  if (m.kind == MOVE_PUSH) {
    push_card_stack(dst, src);
    if (src->id >= STACK_PLAYER_BEGIN) {
      player *pl = s->st.players + (src->id - STACK_PLAYER_BEGIN);
      card_stack *cf = stack_from_id(s, pl->held.camefrom);
      assert (cf->id == pl->held.camefrom);
      unlock_stack(cf);
      if (cf->top > 0) {
        expose_top_card_stack(cf);
      }
    }
  } else { //MOVE_TAKE
    if (src->id == STACK_STOCK) {
      if (src->top == 0) {
        card_stack *waste = stack_from_id(s, STACK_WASTE);
        take_card_stack(waste, src, waste->top);
        for (size_t i = 0; i < src->top; i++) {
          src->s[i].faceup = 0;
        }
      } else {
        expose_top_card_stack(src);
        take_card_stack(src, s->st.board.stacks + STACK_WASTE, 1);
        if (m.stacks.dst >= STACK_PLAYER_BEGIN) {
          s->st.players[m.stacks.dst - STACK_PLAYER_BEGIN].held.camefrom = src->id;
        }
      }
    } else {
      lock_stack(src, m.stacks.dst - STACK_PLAYER_BEGIN);
      take_card_stack(src, dst, m.num_cards);
      if (m.stacks.dst >= STACK_PLAYER_BEGIN) {
        s->st.players[m.stacks.dst - STACK_PLAYER_BEGIN].held.camefrom = src->id;
      }
    }
  }

  update_stack_dims(s, src);
  update_stack_dims(s, dst);

  if (is_solved(s)) {
    s->st.stage = STAGE_WON;
  }

  if (s->sdata.server) {
    s->net.tx.kind = p->kind;
    s->net.tx.data_len = pack(s->net.tx.data, 1, "m", &m);
    broadcast(s);
  }
}


void h_cancel(solr *s, netpacket *p) {
  int8_t uid;
  // needed for client action-matching
  unpack(p->data, 1, "C", (uint8_t*)&uid);
  if (s->sdata.server) uid = true_uid(s);
  if (uid < 0) return; // FIXME: add error reporting
  player *pl = s->st.players + uid;
  if (pl->valid && pl->held.stack.top > 0) {
    card_stack *cf = stack_from_id(s, pl->held.camefrom);
    push_card_stack(cf, &pl->held.stack);
    unlock_stack(cf);
    update_stack_dims(s, cf);
  }
  if (s->sdata.server) {
    s->net.tx.kind = p->kind;
    s->net.tx.data_len = pack(s->net.tx.data, 1, "C", &uid);
    broadcast(s);
  }
}

void h_pos(solr *s, netpacket *p) {
  int8_t uid;
  ivec pos;
  unpack(p->data, 8, "QQC", &pos.x, &pos.y, (uint8_t*)&uid);
  if (s->sdata.server) {
    // FIXME/TODO: secure the bot
    if (uid != SOLR_AUTO_PLAYER) {
      uid = true_uid(s);
    }
  }
  if (uid < 0) return; // FIXME: add error reporting
  s->st.players[uid].pos = pos;
  
  if (s->sdata.server) {
    s->net.tx.kind = p->kind;
    s->net.tx.data_len = pack(s->net.tx.data, 8, "QQC", &pos.x, &pos.y, (uint8_t*)&uid);
    broadcast(s);
  }
}

void h_stdump(solr *s, netpacket *p) {
  gamestate nst = {0};
  unpack(p->data, 8, "G", &nst);
  s->st = nst;
  graphics_resize(s, s->rdr.win_dims);
}

void h_restartreq(solr *s, netpacket *p) {
  if (s->sdata.server) {
    s->st.stage = STAGE_PLAYING;
    setup_board_stacks(s);
    s->net.tx.kind = PCKT_STDUMP;
    s->net.tx.data_len = pack(s->net.tx.data, 8, "G", &s->st);
    broadcast(s);
  }
}

void h_ffwreq(solr *s, netpacket *p) {
  if (s->sdata.server) {
    if (is_ffw_able(s)) {
      ffw(s);
      printf("ffwarding\n");
    } else {
      printf("not ffwable\n");
    }
  }
}

void h_ffwanim(solr *s, netpacket *p) {
  ffw_anim an = {0};
  unpack(p->data, 8, "a", &an);
  ffw_animate_client(s, &an);
}

void h_unrecognized(solr *s, netpacket *p) {
  printf("got unhandled packet kinded %u len %zu\n", p->kind, p->data_len);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winitializer-overrides"
pckt_handler handler_vec[PCKT_MAX] = {
  [0 ... PCKT_MAX-1] = h_unrecognized,
  [PCKT_JOIN] = h_join,
  [PCKT_LEAVE] = h_leave,
  [PCKT_MOVE] = h_move,
  [PCKT_CANCEL] = h_cancel,
  [PCKT_POS] = h_pos,
  [PCKT_STDUMP] = h_stdump,
  [PCKT_RESTARTREQ] = h_restartreq,
  [PCKT_FFWREQ] = h_ffwreq,
  [PCKT_FFWANIM] = h_ffwanim,
};
#pragma GCC diagnostic pop
pckt_handler *pckt_handler_vec_base = handler_vec;

