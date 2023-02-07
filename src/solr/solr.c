#include "solr.h"

void
push_card_stack(card_stack *s, card c) {
  assert(s->top != 13*4);
  s->s[s->top] = c;
  s->top++;
}
card
pop_card_stack(card_stack *s) {
  assert(s->top != 0);
  card c = s->s[s->top];
  s->s[s->top] = (card){0};
  s->top--;
  return c;
}

sch_trans
solr_setup(scheduler *s) {
  setup_data *sdata = (setup_data *)s->setup_data;
  s->persistent_data =
      safe_calloc(sizeof (solr), "setup persistent_data alloc");
  solr *game = (solr *)s->persistent_data;


  for (int suit = 1; suit <= 4; suit++) {
    int clr = suit <= 2;
    for (int rank = 1; rank <= 13; rank++) {
      push_card_stack(&game->st.board.stacks[0],
        (card){.s = suit, .r = rank, .clr = clr});
    }
  }

  game->sdata = *sdata;

  game->net.sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (game->net.sock_fd == -1) {
    perror("Couldn't create a network socket");
    goto neterr;
  }
  fcntl(game->net.sock_fd, F_SETFL, O_NONBLOCK);

  struct sockaddr_in *saddr = &game->sdata.server_a;
  size_t addrl = sizeof (*saddr);
  if (game->sdata.server) {
    saddr->sin_addr.s_addr = INADDR_ANY;

    int retc = bind(game->net.sock_fd, (struct sockaddr *)saddr, addrl);
    if (retc < 0) {
      perror("Couldn't bind network socket");
      goto neterr;
    }

    printf("server running on port %d...\n", SOLR_PORT);
  } else { // client

    if (connect(game->net.sock_fd, (struct sockaddr *)saddr, addrl) < 0) {
      perror("Couldn't connect to server");
      goto neterr;
    }

    // announce our presence
    game->net.peer = game->sdata.server_a;
    game->net.tx = (packet){0};
    game->net.tx.kind = PCKT_JOIN;
    game->net.tx.data_len = pack(game->net.tx.data, 4, MACROSTR(CROK_PLAYER_NAME_MAXLEN) "s", game->sdata.username);
    netmgr_send(&game->net);

    int wait = 0;
    do {
      usleep(5000);
      wait++;
      netmgr_recv(&game->net);
    } while(game->net.rx.kind != CROK_NET_ACK && wait < 50);
    if (wait >= 50) {
      //crok_net_print_packet(stdout, &d->net.rx);
      fprintf(stderr, "couldn't log into lobby!\n"); goto neterr;
    }

    //game->client.uid = server_sockaddr2uid(&game->srv, &game->net.peer);
    unpack(game->net.rx.data, 1, "C", &game->client.uid);
    if (game->client.uid == 0xff) { fprintf(stderr, "game full. quitting\n"); goto neterr; }
    
    char ip4[INET_ADDRSTRLEN];
    printf("using server %s on port %d...\n",
           inet_ntop(AF_INET, &saddr->sin_addr, ip4, INET_ADDRSTRLEN),
           SOLR_PORT);
  }

  return (sch_trans){.s = SCHED_CONTINUE, .why = "setup finished"};
neterr:
  return (sch_trans){.s = SCHED_ERR, .why = "network setup error"};
}

sch_trans
solr_cleanup(scheduler *s) {
  free(s->persistent_data);
  s->persistent_data = NULL;

  return (sch_trans){.s = SCHED_HALT, .why = "cleanup finished"};
}

static ssize_t
true_uid(solr *s) {
  return server_sockaddr2uid(&s->srv, &s->net.peer);
}

static void
broadcast(solr *s) {
  for (int i = 0; i < SOLR_MAX_PLAYERS; i++) {
    if (!s->st.players[i].valid) continue;
    s->net.peer = s->srv.socks[i];
    netmgr_send(&s->net);
  }
}

static void
handle_pckt(solr *s, packet *p) {
  switch (p->kind) {
    case PCKT_JOIN: {
      if (!s->sdata.server) return;
      unsigned char vacant = 0xff;
      for (int i = 0; i < SOLR_MAX_PLAYERS; i++) {
        if (!s->st.players[vacant].valid) {
          vacant = i;
          break;
        }
      }
      if (vacant == 0xff) {} // TODO

      s->st.players[vacant] =
          (player){.valid = 1, .pos = {0}, .username = "anon", .held_card = {0}};
        
      s->srv.socks[vacant] = s->net.peer;
        
      if (p->data_len > 2 && p->data_len < SOLR_NAME_LEN + 2) {
        unpack(p->data, 4, "s", &s->st.players[vacant].username);
      }

      s->net.tx.kind = PCKT_ACK;
      s->net.tx.data_len = pack(s->net.tx.data, 1, "C", vacant);
      netmgr_send(&s->net);

      // TODO / FIXME
      //s->net.tx.kind = PCKT_STDUMP;
      //s->net.tx.data_len = pack(s->net.tx.data, 1, "C", vacant);
      //netmgr_send(&s->net);
    
    break;
    }
  case PCKT_LEAVE: {
    if (!s->sdata.server) return;
    unsigned char uid = true_uid(s);
    if (uid < 0) return;
    s->srv.socks[uid] = (struct sockaddr_in){0};
    s->st.players[uid].valid = 0;
    break;
  }
  case PCKT_PUSH:
  case PCKT_POP: {
    unsigned char uid;
    unsigned char stack_idx;
    unpack(s->net.rx.data, 1, "CC", &uid, &stack_idx);
    if (s->sdata.server) uid = true_uid(s);

    if (p->kind == PCKT_PUSH) {
      push_card_stack(s->st.board.stacks + stack_idx, s->st.players[uid].held_card);
      s->st.players[uid].held_card = (card){0};
    } else {
      s->st.players[uid].held_card =
        pop_card_stack(s->st.board.stacks + stack_idx);
    }
    
    if (s->sdata.server) {
      s->net.tx.kind = p->kind;
      s->net.tx.data_len = pack(s->net.tx.data, 1, "CC", &uid, &stack_idx);
      broadcast(s);
    }
    break;
  }
  case PCKT_POS: {
    unsigned char uid;
    ivec pos;
    unpack(s->net.rx.data, 8, "vC", &pos, &uid);
    if (s->sdata.server) uid = true_uid(s);

    s->st.players[uid].pos = pos;

    if (s->sdata.server) {
      s->net.tx.kind = p->kind;
      s->net.tx.data_len = pack(s->net.tx.data, 8, "vC", &pos, &uid);
      broadcast(s);
    }
    break;
  }
  default: break;
  }
}

sch_trans
solr_fixed(scheduler *s, long step_ns) {
  solr *game = (solr *)s->persistent_data;
  //solr_rdr *rdr = &d->renderdata;
  player *my_player = game->st.players + game->client.uid;
  game->st.tick += 1;
  /* if (d->ticknum * step_ns >= 10 * (long)1e9) { */
  /*   return (sch_trans){.s = SCHED_CLEANUP, */
  /*                               .why = "60 seconds elapsed"}; */
  /* } */

  // twice the size to never overrun with padding
  static pbuf_t stateser[sizeof(struct crok_state)*2];
  //size_t len = crok_state_pack(stateser, st);
  //d->statesum = crc_32((unsigned char *)stateser, len);

  while(1) {
    const char* reject_reason;

    netmgr_recv(&game->net);
    packet *pckt = &game->net.rx;

    // sanitization
    /*
    if (pckt->conn_uid >= 4) {
      reject_reason = "connection unique id out of bounds for packet:";
      goto reject_packet;
    }
    if (pckt->kind >= CROK_NET_END) {
      reject_reason = "packet kind out of bounds for packet:";
      goto reject_packet;
    }
    */
    if (pckt->data_len > SOLR_PACKET_MAX_PAYLOAD_LEN) {
      reject_reason = "packet too large:";
      goto reject_packet;
    }
    
    handle_pckt(game, pckt);

    continue;
    reject_packet:
      fputs(reject_reason, stderr);
      //crok_net_print_pcktrecord(stderr, r);
  }
  
  if (game->sdata.server) goto end;

  SDL_Event input;
  while (SDL_PollEvent(&input) > 0) {
    switch (input.type) {
    case SDL_QUIT:
      printf("leaving...\n");
      game->net.tx = (packet){0};
      game->net.tx.kind = PCKT_LEAVE;
      game->net.peer = game->sdata.server_a;
      netmgr_send(&game->net);
      break;
    case SDL_MOUSEMOTION: {
        struct ivec pos = {.x = input.motion.x, .y = input.motion.y};
	      game->net.tx.kind = PCKT_POS;
      	game->net.tx.data_len = pack(game->net.tx.data, 8, "vC", &pos, &game->client.uid);
        netmgr_send(&game->net);
      }
      break;
    /*
    case SDL_MOUSEBUTTONUP:
      break;
    case SDL_MOUSEBUTTONDOWN:
      nc->tx.kind = CROK_NET_CLICK;
      nc->tx.data_len = 0;
      crok_net_tx_to_server(nc);
      break;
    case SDL_KEYDOWN:
      switch (input.key.keysym.sym) {
        case SDLK_RETURN:
        case SDLK_RETURN2:
        case SDLK_KP_ENTER:
          nc->tx.kind = CROK_NET_READY;
          nc->tx.conn_uid = d->my_player_id;
          nc->tx.ticknum = d->ticknum;
          nc->tx.statesum = d->statesum;
          nc->tx.data_len = 0;
          crok_net_tx_to_server(nc);
          break;
        case SDLK_BACKSPACE:
          nc->tx.kind = CROK_NET_CANCEL;
          nc->tx.conn_uid = d->my_player_id;
          nc->tx.ticknum = d->ticknum;
          nc->tx.statesum = d->statesum;
          nc->tx.data_len = 0;
      }
    */
    default:
      break;
    }
  }
  
  end:
  return (sch_trans){.s = SCHED_CONTINUE, .why = "fixed call"};
}

void
solr_render(scheduler *s, double a) {
  solr *game = (struct crok_data *)s->persistent_data;
  solr_rdr *r = &d->renderdata;

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

// FIXME: player held_card should be held_stack
// TODO: game state serde
// TODO: stack randomization
// TODO: all the rendering code
