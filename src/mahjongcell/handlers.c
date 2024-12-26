#include "solr.h"
#include "mahjcell_net.h"

void h_join(solr *s, netpacket *p) {
  if (!s->sdata.server) return;

  // FIXME: duplicate code shared with accept branch of fixed
  unsigned char vacant = 0xff;
  for (int i = 0; i < SOLR_MAX_PLAYERS; i++) {
    if (!s->st.players[i].valid) {
      vacant = i;
      break;
    }
  }
  if (vacant == 0xff) { return; } // TODO

  s->st.players[vacant].valid = 1;
  s->st.players[vacant].pos = (fvec){0};
    
  if (p->data_len > 4 && p->data_len < SOLR_NAME_LEN + 4) {
    unpack(p->data, 4, "s", &s->st.players[vacant].username);
  }

  s->net.sock_fd = s->srv.sock_fds[vacant];
  s->net.tx->data_len = pack(s->net.tx->data, 1, "C", &vacant);
  mahjcell_send(s, PCKT_ACK);
  mahjcell_send(s, PCKT_STDUMP);
}

void h_leave(solr *s, netpacket *p) {
  int8_t uid;
  if (!s->sdata.server) return;
  uid = true_uid(s);
  if (uid < 0) return;
  s->srv.sock_fds[uid] = 0;
  s->srv.sock_addresses[uid] = (struct sockaddr_storage){0};
  s->st.players[uid].valid = 0;
}

void h_disconnect(solr *s, netpacket *p) {
  h_leave(s, p);
}

void h_ffwreq(solr *s, netpacket *p);
void h_move(solr *s, netpacket *p) {
  card_stack *stacks = s->st.board.stacks;
  move m;
  char *rej_reason;
  unpack(p->data, 1, "[C]", &move_serde, &m);

  printf("got move %s\n", move_repr(s, &m));

  if (!move_valid(s, &m, &rej_reason)) {
    printf("invalid move: %s\n", rej_reason);
    return;
  }

  // TODO: maybe restrict moves coming from players
  // as to not touch other players held stacks?
  if (m.kind == MOVE_GROUP_DRAGONS) {
    int slot_found = 0;
    int target_slot = -1;
    for (int i = STACK_HOLDING_1; !slot_found && i <= STACK_HOLDING_3; i++) {
      card_stack *hs = stack_from_id(s, i);
      if (is_locked_stack(hs)) continue;
      if (hs->top > 0) {
        card *d_c = hs->s + hs->top - 1;
        if (d_c->r == RANK_DRAGON && d_c->s == m.suit) {
          target_slot = i;
        }
      } else {
        target_slot = i; // empty ones are fair game
      }
    }
    if (target_slot < 0) goto finish_move;
    card_stack *target_stack = stack_from_id(s, target_slot);
    // search everything but the player hands
    for (int i = 0; i < STACK_BOARD_MAX; i++) {
      card_stack *dragon_src = stack_from_id(s, i);
      if (is_locked_stack(dragon_src)) continue;
      if (dragon_src == target_stack) continue;
      card *d_c = dragon_src->s + dragon_src->top - 1;
      if (d_c->r == RANK_DRAGON && d_c->s == m.suit) {
        take_card_stack(dragon_src, target_stack, 1);
        update_stack_dims(s, dragon_src);
      }
    }
    update_stack_dims(s, target_stack);
  } else {
    card_stack *src = stack_from_id(s, m.stacks.src);
    card_stack *dst = stack_from_id(s, m.stacks.dst);

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
    } else if (MOVE_TAKE) {
      lock_stack(src, m.stacks.dst - STACK_PLAYER_BEGIN);
      take_card_stack(src, dst, m.num_cards);
      if (m.stacks.dst >= STACK_PLAYER_BEGIN) {
        s->st.players[m.stacks.dst - STACK_PLAYER_BEGIN].held.camefrom = src->id;
      }
    }

    update_stack_dims(s, src);
    update_stack_dims(s, dst);
  }

finish_move:
  if (is_solved(s)) {
    s->st.stage = STAGE_WON;
  }

  if (s->sdata.server) {
    s->net.tx->data_len = pack(s->net.tx->data, 1, "[C]", &move_serde, &m);
    mahjcell_broadcast(s, p->kind);
  }

  h_ffwreq(s, NULL);
}

void h_cancel(solr *s, netpacket *p) {
  int8_t uid;
  // needed for client action-matching
  unpack(p->data, 1, "C", (uint8_t*)&uid);
  if (s->sdata.server) {
    int8_t actual_uid = true_uid(s);
    if (actual_uid != uid) {
      fprintf(stderr,
          "%s: reported uid doesn't match"
          "source uid! (%hhd != %hhd)\n",
        __func__, actual_uid, uid);
    }
    uid = actual_uid;
  }
  if (uid < 0) return; // FIXME: add error reporting
  player *pl = s->st.players + uid;
  if (pl->valid && pl->held.stack.top > 0) {
    card_stack *cf = stack_from_id(s, pl->held.camefrom);
    push_card_stack(cf, &pl->held.stack);
    unlock_stack(cf);
    update_stack_dims(s, cf);
  }
  if (s->sdata.server) {
    s->net.tx->data_len = pack(s->net.tx->data, 1, "C", &uid);
    mahjcell_broadcast(s, p->kind);
  }
}

void h_pos(solr *s, netpacket *p) {
  int8_t uid;
  fvec pos;
  unpack(p->data, 8, "IIC", &pos.x, &pos.y, (uint8_t*)&uid);
  if (s->sdata.server) {
    // FIXME/TODO: secure the bot
    if (uid != SOLR_AUTO_PLAYER) {
      uid = true_uid(s);
    }
  }
  if (uid < 0) return; // FIXME: add error reporting
  s->st.players[uid].pos = pos;
  
  if (s->sdata.server) {
    s->net.tx->data_len = pack(s->net.tx->data, 8, "IIC", &pos.x, &pos.y, (uint8_t*)&uid);
    mahjcell_broadcast(s, p->kind);
  }
}

void h_stdump(solr *s, netpacket *p) {
  if (s->sdata.server) {
    fprintf(stderr, "received client %d's statedump(?)\n", true_uid(s));
    return;
  }
  fprintf(stderr, "st_dump recv and processed\n");
  gamestate nst = {0};
  unpack(p->data, 8, "G", &nst);
  s->st = nst;
  graphics_resize(s, s->rdr.win_dims);
}

void h_restartreq(solr *s, netpacket *p) {
  if (s->sdata.server) {
    s->st.stage = STAGE_PLAYING;
    setup_board_stacks(s);
    mahjcell_broadcast(s, PCKT_STDUMP);
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
  [PCKT_DISCONNECT] = h_disconnect,
};
#pragma GCC diagnostic pop
pckt_handler *pckt_handler_vec_base = handler_vec;

