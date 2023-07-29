#include "solr.h"
#include "ivec.h"
#include "net.h"

static void
serde_init() {
  serde_fun *t = serializer_table;
  t['b'] = board_serde;
  t['c'] = card_serde;
  t['f'] = card_stack_serde;
  t['m'] = move_serde;
  t['p'] = player_serde;
  t['G'] = gamestate_serde;
  t['a'] = ffw_anim_serde;
}

size_t
board_serde(pbuf_t *buf, char mode, void *vb) {
  board *b = vb;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(board);
  return pack_unpack(buf, mode, 8, "14f", &b->stacks);
}

size_t
player_serde(pbuf_t *buf, char mode, void *vp) {
  player *p = vp;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(player);
  return pack_unpack(buf, mode, 8, "QQ" MACROSTR(SOLR_NAME_LEN) "sfCC",
    &p->pos.x, &p->pos.y, &p->username, &p->held.stack, &p->held.camefrom, &p->valid);
}

size_t
gamestate_serde(pbuf_t *buf, char mode, void *vst) {
  gamestate *st = vst;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(gamestate);
  return pack_unpack(buf, mode, 8, "Cb" MACROSTR(SOLR_MAX_PLAYERS) "pQ",
    &st->stage, &st->board, &st->players, &st->tick);
}

SDL_Rect
posdims2rect(ivec pos, ivec dims) {
  return (SDL_Rect){pos.x, pos.y, dims.x, dims.y};
}

void
broadcast(solr *s) {
  for (int i = 0; i < SOLR_REAL_PLAYERS; i++) {
    if (!s->st.players[i].valid) continue;
    s->net.peer = s->srv.socks[i];
    netmgr_send(&s->net);
  }
}

static void
fling_suits(solr *s) {
  if (s->st.flung_card.valid) return;
  uint8_t rsid = (rand() % (STACK_SUITS_END - STACK_SUITS_BEGIN + 1)) + STACK_SUITS_BEGIN;
  card_stack *fs = stack_from_id(s, rsid);
  if (fs->top == 0) return;
  card c = fs->s[fs->top - 1];
  fs->s[fs->top - 1] = (card){0};
  fs->top--;

  physical_card pc = {
    .pb.pos.x = s->rdr.stack_areas[fs->id].x,
    .pb.pos.y = s->rdr.stack_areas[fs->id].y,
    .pb.vel = (ivec){.x = rand() % 20 - 10, .y = 0},
    .pb.acc = ivec_zero(),
    .c = c,
    .valid = 1,
  };
  s->st.flung_card = pc;
}

static int flung_behaviour(solr *s) {
  physical_card *pc = &s->st.flung_card;
  solr_rdr *rd = &s->rdr;
  if (!pc->valid) return -1;
  int64_t x = pc->pb.pos.x;
  int64_t y = pc->pb.pos.y;
  if (x < -rd->card_dims.x || x > (int64_t)rd->win_dims.x
    || (y == rd->win_dims.y - rd->card_dims.y && pc->pb.vel.y < 2)
  ) {
    s->st.flung_card.valid = 0;
    return -1;
  }

  physbody_accel(&pc->pb, (ivec){0,1});
  physbody_tick(&pc->pb);
  if (y + rd->card_dims.y > rd->win_dims.y) {
    physbody_accel(&pc->pb, (ivec){0, -pc->pb.vel.y - pc->pb.vel.y / 2});
    pc->pb.pos.y = rd->win_dims.y - rd->card_dims.y;
  }
  return 0;
}

static ssize_t
stack_bbox(solr *s, ivec mpos) {
  for (size_t i = 0; i < STACK_BOARD_MAX; i++) {
    SDL_Rect box = s->rdr.stack_areas[i];
    SDL_Point p = {.x = mpos.x, .y = mpos.y};
    printf("stack bbox test m %d,%d b %d,%d,%d,%d\n",
      p.x,
      p.y,
      box.x,
      box.y,
      box.w,
      box.h
      );
    if (SDL_PointInRect(&p, &box) == SDL_TRUE) {
      return i;
    }
  }
  return -1;
}

static ssize_t
card_bbox(solr *s, size_t stack_idx, ivec mpos) {
  solr_rdr *rd = &s->rdr;
  assert(stack_idx < STACK_BOARD_MAX);
  SDL_Rect *area = rd->stack_areas + stack_idx;
  ivec pos = {area->x, area->y};
  ivec s_dims = {area->w, area->h};
  card_stack *ss = s->st.board.stacks + stack_idx;
  if (ss->top == 0) goto not_found;

  int ystep = (s_dims.y - rd->card_dims.y)/(ss->top - 1 * (ss->top > 1));
  ivec cpos = {.x = pos.x, .y = pos.y + ystep * (ss->top - 1)};
  for (ssize_t i = ss->top - 1; i >= (ss->splayed ? 0 : ss->top - 1); i--) {
    SDL_Rect box = posdims2rect(cpos, rd->card_dims);
    SDL_Point p = {.x = mpos.x, .y = mpos.y};
    printf("bbox test m %d,%d b %d,%d,%d,%d\n",
      p.x,
      p.y,
      box.x,
      box.y,
      box.w,
      box.h
      );
    if (SDL_PointInRect(&p, &box) == SDL_TRUE) {
      return i;
    }
    cpos.y -= ystep;
  }

  // returning zero is technically incorrect,
  // as it is a valid index. but it's easier this way
  not_found: return 0;
}

static void
setup_board_stacks(solr *game) {
  for (size_t i = STACK_TABLEAU_BEGIN; i <= STACK_TABLEAU_END; i++) {
    game->st.board.stacks[i].splayed = 1;
  }
  for (size_t i = 0; i < SOLR_MAX_PLAYERS; i++) {
    game->st.players[i].held.stack.splayed = 1;
  }

  card_stack *stock = stack_from_id(game, STACK_STOCK);
  card_stack *waste = stack_from_id(game, STACK_WASTE);
  stock->top = 0;
  stock->pile = 1;
  waste->top = 0;
  waste->pile = 1;
  for (int suit = SUIT_BEG; suit < SUIT_MAX; suit++) {
    int clr = suit <= 2;
    for (int rank = RANK_BEG; rank < RANK_MAX; rank++) {
      stock->s[stock->top] = (card){.s = suit, .r = rank, .faceup = 0, .clr = clr};
      stock->top++;
    }
  }

  shuffle_stack(stock);
  
  for (int i = STACK_TABLEAU_BEGIN; i <= STACK_TABLEAU_END; i++) {
    card_stack *col = stack_from_id(game, i);
    col->top = 0;
    for (int j = 0; j < i; j++) {
      take_card_stack(stock, col, 1);
    }
    expose_top_card_stack(col);
  }

  for (int i = STACK_SUITS_BEGIN; i <= STACK_SUITS_END; i++) {
    game->st.board.stacks[i].top = 0;
  }

  for (size_t i = 0; i < STACK_MAX; i++) {
    card_stack *s = stack_from_id(game, i);
    update_stack_dims(game, s);
    s->id = i;
  }
}

static void almost_solved_stacks(solr *game) {
  for (size_t i = STACK_TABLEAU_BEGIN; i <= STACK_TABLEAU_END; i++) {
    game->st.board.stacks[i].splayed = 1;
  }
  for (size_t i = 0; i < SOLR_MAX_PLAYERS; i++) {
    game->st.players[i].held.stack.splayed = 1;
  }

  card_stack *stock = stack_from_id(game, STACK_STOCK);
  stock->pile = 1;
  for (int suit = SUIT_BEG; suit < SUIT_MAX; suit++) {
    int clr = suit <= 2;
    card_stack *cs = &game->st.board.stacks[STACK_SUITS_BEGIN + suit - SUIT_BEG];
    for (int rank = RANK_BEG; rank < RANK_MAX; rank++) {
      cs->s[cs->top] = (card){.s = suit, .r = rank, .faceup = 1, .clr = clr};
      cs->top++;
    }
  }

  take_card_stack(stack_from_id(game, STACK_SUITS_BEGIN), stock, 1);
  stock->s[stock->top - 1].faceup = 0;
  
  for (size_t i = 0; i < STACK_MAX; i++) {
    card_stack *s = stack_from_id(game, i);
    update_stack_dims(game, s);
    s->id = i;
  }
}

static int
is_solved(solr *s) {
  size_t c_suits = 0;
  for (size_t i = STACK_SUITS_BEGIN; i <= STACK_SUITS_END; i++) {
    card_stack *cs = s->st.board.stacks + i;
    c_suits += cs->top;
  }
  return c_suits >= 52;
}

sch_trans
solr_setup(scheduler *s) {
  setup_data *sdata = (setup_data *)s->setup_data;
  s->persistent_data =
      safe_calloc(sizeof (solr), "setup persistent_data alloc");
  solr *game = (solr *)s->persistent_data;

  serde_init();
  srand(time(NULL));

  game->sdata = *sdata;

  #if 1

  WSADATA wsaData;
  WORD vReq = MAKEWORD(2,2);
  int err = WSAStartup(vReq, &wsaData);
  if (err != 0) {
    printf("WSAStartup failed with error: %d\n", err);
    goto neterr;
  }

  #endif
  
  game->net.sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  #if 0
  if (game->net.sock_fd == -1) {
    perror("Couldn't create a network socket");
    goto neterr;
  }
  fcntl(game->net.sock_fd, F_SETFL, O_NONBLOCK);
  #else
  if (game->net.sock_fd == INVALID_SOCKET) {
    printf("Couldn't create a network socket, error num: %d\n", WSAGetLastError());
    goto neterr;
  }
  u_long mode = 1;
  ioctlsocket(game->net.sock_fd, FIONBIO, &mode);
  #endif
  game->net.spkinds.onblock = PCKT_END;
  game->net.spkinds.onerror = PCKT_INVALID;

  struct sockaddr_in *saddr = &game->sdata.server_a;
  size_t addrl = sizeof (*saddr);

  if (game->sdata.server) {
    saddr->sin_addr.s_addr = INADDR_ANY;

    int retc = bind(game->net.sock_fd, (struct sockaddr *)saddr, addrl);
    if (retc < 0) {
      #if 0
      perror("Couldn't bind network socket");
      #else
      printf("Couldn't bind network socket, error num: %d\n", WSAGetLastError());
      #endif
      goto neterr;
    }
  
    //setup_board_stacks(game);
    almost_solved_stacks(game);
    
    printf("server running on port %d...\n", ntohs(sdata->server_a.sin_port));
  } else { // client

    if (connect(game->net.sock_fd, (struct sockaddr *)saddr, addrl) < 0) {
      perror("Couldn't connect to server");
      goto neterr;
    }

    // announce our presence
    game->net.peer = game->sdata.server_a;
    game->net.tx = (netpacket){0};
    game->net.tx.kind = PCKT_JOIN;
    game->net.tx.data_len = pack(game->net.tx.data, 1, MACROSTR(SOLR_NAME_LEN) "s", game->sdata.username);
    netmgr_send(&game->net);

    int wait = 0;
    do {
      usleep(5000);
      wait++;
      netmgr_sprecv(&game->net);
    } while(game->net.rx.kind != PCKT_ACK && wait < 50);
    if (wait >= 50) {
      //crok_net_print_packet(stdout, &d->net.rx);
      fprintf(stderr, "couldn't log into lobby!\n"); goto neterr;
    }

    //game->client.uid = server_sockaddr2uid(&game->srv, &game->net.peer);
    unpack(game->net.rx.data, 1, "C", &game->client.uid);
    if (game->client.uid == 0xff) { fprintf(stderr, "game full. quitting\n"); goto neterr; }
    game->st.players[game->client.uid].valid = 1;
    
    printf("using server %s on port %d...\n",
           inet_ntoa(saddr->sin_addr),
           ntohs(sdata->server_a.sin_port));

    printf("loading graphics...\n");
    graphics_init(game);
  }

  
   
  return (sch_trans){.s = SCHED_CONTINUE, .why = "setup finished"};
neterr:
  return (sch_trans){.s = SCHED_ERR, .why = "network setup error"};
}

sch_trans
solr_cleanup(scheduler *s) {
  solr *game = (solr*)s->persistent_data;
  if (!game->sdata.server) {
    graphics_finish(game);
  }
  free(s->persistent_data);
  s->persistent_data = NULL;

  return (sch_trans){.s = SCHED_HALT, .why = "cleanup finished"};
}

ssize_t server_sockaddr2uid(server *s, struct sockaddr_in *sock) {
  ssize_t loc = -1;
  for (size_t i = 0; i < SOLR_MAX_PLAYERS; i++) {
    if (s->socks[i].sin_addr.s_addr == sock->sin_addr.s_addr
        && s->socks[i].sin_port == sock->sin_port)
    {
      loc = i;
      break;
    }
  }
  return loc;
}

static ssize_t
true_uid(solr *s) {
  return server_sockaddr2uid(&s->srv, &s->net.peer);
}

void
handle_pckt(solr *s, netpacket *p) {
  int8_t uid;
  switch (p->kind) {
    case PCKT_JOIN: {
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
      break;
    }
  case PCKT_LEAVE: {
    if (!s->sdata.server) return;
    uid = true_uid(s);
    if (uid < 0) return;
    s->srv.socks[uid] = (struct sockaddr_in){0};
    s->st.players[uid].valid = 0;
    break;
  }
  case PCKT_MOVE: {
    card_stack *stacks = s->st.board.stacks;
    move m;
    char *rej_reason;
    unpack(p->data, 1, "m", &m);
    if (!move_valid(s, &m, &rej_reason)) {
      printf("invalid move: %s\n", rej_reason);
      break;
    }
    printf("move from %d to %d\n", m.stacks.src, m.stacks.dst);

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

    printf("bef src %d,%d, dst %d,%d\n",
        s->rdr.stack_areas[src->id].w,
        s->rdr.stack_areas[src->id].h,
        s->rdr.stack_areas[dst->id].w,
        s->rdr.stack_areas[dst->id].h
        );

    update_stack_dims(s, src);
    update_stack_dims(s, dst);

    printf("bef src %d,%d, dst %d,%d\n",
        s->rdr.stack_areas[src->id].w,
        s->rdr.stack_areas[src->id].h,
        s->rdr.stack_areas[dst->id].w,
        s->rdr.stack_areas[dst->id].h
        );

    
    if (is_solved(s)) {
      s->st.stage = STAGE_WON;
    }

    if (s->sdata.server) {
      s->net.tx.kind = p->kind;
      s->net.tx.data_len = pack(s->net.tx.data, 1, "m", &m);
      broadcast(s);
    }

    break;
  }
  case PCKT_CANCEL: {
    // needed for client action-matching
    unpack(p->data, 1, "C", (uint8_t*)&uid);
    if (s->sdata.server) uid = true_uid(s);
    if (uid < 0) break;
    player *pl = s->st.players + uid;
    if (pl->valid && pl->held.stack.top > 0) {
      card_stack *cf = stack_from_id(s, pl->held.camefrom);
      push_card_stack(cf, &pl->held.stack);
      unlock_stack(cf);
    }
    if (s->sdata.server) {
      s->net.tx.kind = p->kind;
      s->net.tx.data_len = pack(s->net.tx.data, 1, "C", &uid);
      broadcast(s);
    }
    break;
  }
  case PCKT_POS: {
    ivec pos;
    unpack(p->data, 8, "QQC", &pos.x, &pos.y, (uint8_t*)&uid);
    if (s->sdata.server) {
      // FIXME/TODO: secure the bot
      if (uid != SOLR_AUTO_PLAYER) {
        uid = true_uid(s);
      }
    }
    if (uid < 0) break;
    s->st.players[uid].pos = pos;
    
    if (s->sdata.server) {
      s->net.tx.kind = p->kind;
      s->net.tx.data_len = pack(s->net.tx.data, 8, "QQC", &pos.x, &pos.y, (uint8_t*)&uid);
      broadcast(s);
    }
    break;
  }
  case PCKT_STDUMP: {
    gamestate nst = {0};
    unpack(p->data, 8, "G", &nst);
    s->st = nst;
    graphics_resize(s, s->rdr.win_dims);
    break;
  }
  case PCKT_RESTARTREQ: {
    if (s->sdata.server) {
      s->st.stage = STAGE_PLAYING;
      setup_board_stacks(s);
      s->net.tx.kind = PCKT_STDUMP;
      s->net.tx.data_len = pack(s->net.tx.data, 8, "G", &s->st);
      broadcast(s);
      printf("got restart\n");
    }
    break;
  }
  case PCKT_FFWREQ: {
    if (s->sdata.server) {
      if (is_ffw_able(s)) {
        ffw(s);
        printf("ffwarding\n");
      } else {
        printf("not ffwable\n");
      }
    }
    break;
  }
  case PCKT_FFWANIM: {
    ffw_anim an = {0};
    unpack(p->data, 8, "a", &an);
    ffw_animate_client(s, &an);
    break;
  }
  default: {
    printf("got unhandled packet kinded %u len %zu\n", p->kind, p->data_len);
    break;
  }
  }
}

float
lerp(float a, float b, float v) {
  return (a * (1.0 - v) + (b * v));
}

sch_trans
solr_fixed(scheduler *s, long step_ns) {
  solr *game = (solr *)s->persistent_data;
  player *my_player = game->st.players + game->client.uid;
  game->st.tick += 1;

  if (game->st.stage == STAGE_FFW) {
    ffw_animate_server(game);
  }

  // just to not leave unbounded
  for (size_t i = 0; i < 10000; i++) {
    const char* reject_reason;
    netmgr_sprecv(&game->net);
    netpacket *pckt = &game->net.rx;

    if (pckt->kind == PCKT_END) break;
    if (pckt->data_len > NETPACKET_MAX_PAYLOAD) {
      reject_reason = "packet too large:";
      goto reject_packet;
    }
    
    handle_pckt(game, pckt); 

    continue;
    reject_packet:
      fputs(reject_reason, stderr);
  }

  // if (game->sdata.server && game->st.tick % 200 == 0) {
  //   game->net.tx.kind = PCKT_STDUMP;
  //   game->net.tx.data_len = pack(game->net.tx.data, 8, "G", &game->st);
  //   broadcast(game);
  // }
  
  if (game->sdata.server) goto end;
  
  if (game->st.stage == STAGE_WON) {
    game->st.players[SOLR_AUTO_PLAYER].valid = 0;
    if (flung_behaviour(game) == -1) {
      fling_suits(game);
    }
  }
  
  static ivec mouse_pos = {0};
  SDL_Event input;
  while (SDL_PollEvent(&input) > 0) {
    game->net.tx = (netpacket){0};
    game->net.peer = game->sdata.server_a;

    switch (input.type) {
    case SDL_WINDOWEVENT: {
      switch (input.window.event)
      case SDL_WINDOWEVENT_RESIZED:
      case SDL_WINDOWEVENT_SIZE_CHANGED: {
        ivec newsize = {input.window.data1, input.window.data2};
        graphics_resize(game, newsize);
        printf("resized to %ldx%ld\n", newsize.x, newsize.y);
        break;
      }
      break;
    }
    case SDL_QUIT:
      printf("leaving...\n");
      game->net.tx.kind = PCKT_LEAVE;
      netmgr_send(&game->net);
      goto leave;
    case SDL_MOUSEMOTION: {
        mouse_pos = (ivec){.x = input.motion.x, .y = input.motion.y};
        struct ivec pos = {.x = input.motion.x, .y = input.motion.y};
	      game->net.tx.kind = PCKT_POS;
      	game->net.tx.data_len = pack(game->net.tx.data, 8, "QQC", &pos.x, &pos.y, &game->client.uid);
        netmgr_send(&game->net);
      }
      break;
    
    case SDL_MOUSEBUTTONUP: {
      player *myp = game->st.players + game->client.uid;
      if (input.button.button == SDL_BUTTON_RIGHT) {
        game->net.tx.kind = PCKT_CANCEL;
        game->net.tx.data_len = pack(game->net.tx.data, 1, "C", &game->client.uid);
        netmgr_send(&game->net);
        break;
      }
      ssize_t stack_idx = stack_bbox(game, mouse_pos);
      if (stack_idx < 0) break;
      unsigned char n = game->st.board.stacks[stack_idx].top
          - card_bbox(game, stack_idx, mouse_pos);

      move m = {
        .kind = myp->held.stack.top > 0 ? MOVE_PUSH : MOVE_TAKE,
        .num_cards = n,
      };
      m.stacks.src = m.kind == MOVE_PUSH ? STACK_PLAYER_BEGIN + game->client.uid : stack_idx;
      m.stacks.dst = m.kind == MOVE_TAKE ? STACK_PLAYER_BEGIN + game->client.uid : stack_idx;
      
        if(m.stacks.dst == myp->held.camefrom){
             game->net.tx.kind = PCKT_CANCEL;
        game->net.tx.data_len = pack(game->net.tx.data, 1, "C", &game->client.uid);
        netmgr_send(&game->net);
        break;
        }
      game->net.tx.kind = PCKT_MOVE;
      game->net.tx.data_len = pack(game->net.tx.data, 1, "m", &m);
      netmgr_send(&game->net);
      break;
    }
    
    case SDL_KEYUP:
      switch (input.key.keysym.sym) {
        case SDLK_RETURN:
        case SDLK_RETURN2:
        case SDLK_KP_ENTER:
          break;
        case SDLK_SPACE: {
          move m = {
            .kind = MOVE_TAKE,
            .stacks.src = STACK_STOCK,
            .stacks.dst = STACK_WASTE,
            .num_cards = 1,
          };

          game->net.tx.kind = PCKT_MOVE;
          game->net.tx.data_len = pack(game->net.tx.data, 1, "m", &m);
          netmgr_send(&game->net);
          break;
        }
        case SDLK_r: {
          game->net.tx.kind = PCKT_RESTARTREQ;
          game->net.tx.data_len = 0;
          netmgr_send(&game->net);
          break;
        }
        case SDLK_f: {
          game->net.tx.kind = PCKT_FFWREQ;
          game->net.tx.data_len = 0;
          netmgr_send(&game->net);
          break;
        }
        case SDLK_d: {
          printf("state %d\n", game->st.stage);
          break;
        }
      }
      break;
    default:
      break;
    }
  }
  
  end:
  return (sch_trans){.s = SCHED_CONTINUE, .why = "fixed call"};
  leave:
  return (sch_trans){.s = SCHED_CLEANUP, .why = "user requested quit"};
}

void
player_render(solr_rdr *rdr, player *p) {
  SDL_SetRenderDrawColor(rdr->rdr, 0xff, 0x20, 0x20, 0xb0);
  SDL_RenderFillCircle(rdr->rdr, p->pos.x, p->pos.y, 3);
}

void
card_render(solr_rdr *rdr, card* c, ivec pos) {
  SDL_Rect rect;
  SDL_Rect dst = posdims2rect(pos, rdr->card_dims);
  SDL_Texture **atlas;

  if (!c->faceup) {
    atlas = &rdr->backs;
    rect.x = 0; rect.y = 0;
    rect.w = 88; rect.h = 124;
  }
  else {
    atlas = &rdr->suits[c->s];
    rect.x = ((c->r - 1) % 5) * 88;
    rect.y = ((c->r - 1) / 5) * 124;
    rect.w = 88; rect.h = 124;
  }
  if (SDL_RenderCopy(rdr->rdr, *atlas, &rect, &dst) < 0) {
    fprintf(stderr, "%s: %s", __func__, SDL_GetError());
    SDL_ClearError();
  }
}

void
card_stack_render(solr_rdr *rdr, card_stack *s, ivec pos) {
  SDL_Rect dst = posdims2rect(pos, rdr->card_dims);
  if (s->top == 0) {
    SDL_Rect rect = {.x = 0, .y = 0, .w = 88, .h = 124};
    
    if (SDL_RenderCopy(rdr->rdr, rdr->slot, &rect, &dst) < 0) {
      fprintf(stderr, "%s: %s", __func__, SDL_GetError());
      SDL_ClearError();
    }
    return;
  }
  if (s->pile) {
    SDL_Rect rect = {.x = 0, .y = 0, .w = 88, .h = 124};
    for (ssize_t i = 0; i < s->top; i++) {
      if (i == s->top - 1) { card_render(rdr, &s->s[i], (ivec){dst.x,dst.y}); }
      else if (SDL_RenderCopy(rdr->rdr, rdr->backs, &rect, &dst) < 0) {
        fprintf(stderr, "%s: %s", __func__, SDL_GetError());
        SDL_ClearError();
      }
      dst.y -= 2;
    }
  }

  else if (!s->splayed) {
    card_render(rdr, &s->s[s->top - 1], pos);
  } else {
    for (size_t i = 0; i < s->top; i++) {
      card_render(rdr, &s->s[i], pos);
      pos.y += yoff_card_stack(rdr, s)/(s->top - 1 * (s->top > 1));
    }
  }

  if (is_locked_stack(s)) {
    SDL_Rect rect = {.x = 0, .y = 0, .w = 313, .h = 401};
    #define IW 62
    #define IH 80
    SDL_Rect dst_icon = {.x = dst.x, .y = dst.y, .w = IW, .h = IH};
    dst_icon.x += dst.w/2 - IW/2;
    dst_icon.y += dst.h/2 - IH/2;
    
    SDL_RenderCopy(rdr->rdr, rdr->locked_icon, &rect, &dst_icon);
  }
}

int yoff_card_stack(solr_rdr *rdr, card_stack *s) {
  return (rdr->stack_areas[s->id].h - rdr->card_dims.y);
}

void
solr_render(scheduler *s, double a) {
  solr *game = (solr *)s->persistent_data;
  solr_rdr *r = &game->rdr;

  SDL_SetRenderDrawColor(r->rdr, 0x3c, 0x3c, 0x3c, SDL_ALPHA_OPAQUE);
  if (game->st.stage != STAGE_WON) {
    SDL_RenderClear(r->rdr);
  } else {
    card_render(r, &game->st.flung_card.c, game->st.flung_card.pb.pos);
    goto end;
  }

  for (size_t i = 0; i < STACK_BOARD_MAX; i++) {
    card_stack_render(r, game->st.board.stacks + i, (ivec){r->stack_areas[i].x, r->stack_areas[i].y});
  }

  for (size_t i = 0; i < SOLR_MAX_PLAYERS; i++) {
    player *p = game->st.players + i;
    if (p->valid) {
      if (p->held.stack.top > 0){
        ivec offset = ivec_sub(p->pos, ivec_div(r->card_dims, (ivec){2,2}));
        card_stack_render(r, &p->held.stack, offset);
      }
      player_render(r, p);
    }
  }

  // triggers the double buffers for multiple rendering
  end: SDL_RenderPresent(r->rdr);
}

// FIXME: player held_card should be held_stack
// TODO: game state serde
// TODO: stack randomization
// TODO: disconnect on win (or do something better)
// TODO: figure out why after a win restarts don't work

