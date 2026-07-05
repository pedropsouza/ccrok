#include "solr.h"
#include "ivec.h"
#include "net.h"
#include "mahjcell_net.h"

static void
serde_init() {
  serde_fun *t = serializer_table;
  t['b'] = board_serde;
  t['c'] = card_serde;
  t['f'] = card_stack_serde;
  t['p'] = player_serde;
  t['G'] = gamestate_serde;
  t['a'] = ffw_anim_serde;
}

size_t
board_serde(pbuf_t *buf, char mode, void *vb) {
  board *b = vb;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(board);
  return pack_unpack(buf, mode, 8, "15f", &b->stacks);
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
    .pb.vel = (fvec){.x = rand() % 20 - 10, .y = 0},
    .pb.acc = fvec_zero(),
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

  physbody_accel(&pc->pb, (fvec){0,1});
  physbody_tick(&pc->pb);
  if (y + rd->card_dims.y > rd->win_dims.y) {
    physbody_accel(&pc->pb, (fvec){0, -pc->pb.vel.y - pc->pb.vel.y / 2});
    pc->pb.pos.y = rd->win_dims.y - rd->card_dims.y;
  }
  return 0;
}

static ssize_t
stack_bbox(solr *s, ivec mpos) {
  for (size_t i = 0; i < STACK_BOARD_MAX; i++) {
    SDL_Rect box = s->rdr.stack_areas[i];
    SDL_Point p = {.x = mpos.x, .y = mpos.y};
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
  ssize_t tail = ss->top - 1;
  if (!ss->splayed && !ss->pile) {
    // must be a suit "pile"
    return tail;
  }

  int ystep = ss->pile ? -2 : rd->card_dims.y/4;
  for (ssize_t i = tail; i >= (ss->splayed ? 0 : tail); i--) {
    ivec cpos = {.x = pos.x, .y = pos.y + ystep * i};
    SDL_Rect box = posdims2rect(cpos, rd->card_dims);
    SDL_Point p = {.x = mpos.x, .y = mpos.y};
    if (SDL_PointInRect(&p, &box) == SDL_TRUE) {
      return i;
    }
  }

  // returning zero is technically incorrect,
  // as it is a valid index. but it's easier this way
  not_found: return 0;
}

int stack_sequence_depth(card_stack *s) {
  if (s->top <= 1) return 1; // base case
  for (int i = s->top - 1; i > 0; i--) {
    card *lo = s->s + i;
    card *hi = s->s + i - 1;
    bool ordered = 1
      && lo->r != RANK_DRAGON
      && hi->r != RANK_DRAGON
      && lo->r + 1 == hi->r
      && lo->s != hi->s;
    if (!ordered) return s->top - i;
  }
  return s->top;
}

const char *
packet_kind_to_name(int kind) {
  #define X(name, num_expr) case num_expr: return #name;
  switch (kind) {
    PACKET_KINDS
    default: return "unknown / undefined";
  };
  #undef X
}

char *
stack_name_from_id(int id) {
  static char buf[2][24] = { "", "" };
  static char buf_idx = 0;

  char *choice;
  if (id >= STACK_MAX) {
    return "unknown (out of range)"; 
  }
  // can't use a switch here since stack ids don't have unique names necessarily
  #define X(name, sid) if (id >= sid) snprintf(buf[buf_idx], 24, #name " #%d", id - sid);
  STACKS_XLIST
  #undef X
  choice = buf[buf_idx];
  buf_idx = (buf_idx+1)%2;
  return choice;
}

void
setup_board_stacks(solr *game) {
  for (size_t i = 0; i <= STACK_BOARD_END; i++) {
    game->st.board.stacks[i].splayed = 1;
    game->st.board.stacks[i].top = 0;
  }
  for (size_t i = 0; i < SOLR_MAX_PLAYERS; i++) {
    game->st.players[i].held.stack.splayed = 1;
    game->st.board.stacks[i].top = 0;
  }

  card_stack *dealer = stack_from_id(game, STACK_FLOWER);
  dealer->top = 0;
  dealer->pile = 1;
  for (int suit = SUIT_BEG; suit <= SUIT_NORMAL_END; suit++) {
    for (int rank = RANK_BEG; rank < RANK_MAX - 1; rank++) {
      dealer->s[dealer->top] = (card){.s = suit, .r = rank, .faceup = 0};
      dealer->top++;
    }
  }

  for (int dragon_i = 0; dragon_i < 4*3; dragon_i++) {
	  dealer->s[dealer->top] = (card){
		  .s = (dragon_i % 3) + 1,
		  .r = RANK_DRAGON,
		  .faceup = 0,
	  };
	  dealer->top++;
  }

  dealer->s[dealer->top] = (card){.s = SUIT_FLOWER, .r = RANK_1, .faceup = 0};
  dealer->top++;

  shuffle_stack(dealer);
  
  for (int i = 0; i < 5; i++) {
    for (int j = STACK_BOARD_BEGIN; j <= STACK_BOARD_END; j++) {
      expose_top_card_stack(dealer);
      card_stack *col = stack_from_id(game, j);
      take_card_stack(dealer, col, 1);
    }
  }

  for (int i = STACK_SUITS_BEGIN; i <= STACK_SUITS_END; i++) {
    stack_from_id(game, i)->top = 0;
    stack_from_id(game, i)->splayed = 0;
  }
  
  for (int i = STACK_HOLDING_1; i <= STACK_HOLDING_3; i++) {
    stack_from_id(game, i)->top = 0;
    stack_from_id(game, i)->splayed = 0;
  }

  for (size_t i = 0; i < STACK_MAX; i++) {
    card_stack *s = stack_from_id(game, i);
    update_stack_dims(game, s);
    s->id = i;
  }
}

int
is_solved(solr *s) {
  for (size_t i = STACK_BOARD_BEGIN; i <= STACK_BOARD_END; i++) {
    card_stack *cs = stack_from_id(s, i);
    if (cs->top) return false;
  }
  size_t stack_card_count = 0;
  for (size_t i = STACK_SUITS_BEGIN; i <= STACK_SUITS_END; i++) {
    card_stack *cs = stack_from_id(s, i);
    stack_card_count += cs->top;
  }
  return stack_card_count == 27;
}

solr
solr_setup  (setup_data *sdata) {
  solr game_actual = {0};
  solr *game = &game_actual;
  game->net.tx = malloc(sizeof(netpacket));
  game->net.rx = malloc(sizeof(netpacket));
  assert(game->net.tx && game->net.rx);
  netpacket *tx = (netpacket *)game->net.tx;
  netpacket *rx = (netpacket *)game->net.rx;

  serde_init();
  srand(time(NULL));

  game->sdata = *sdata;

  game->net.sock_fd =
    //socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (game->net.sock_fd == -1) {
    perror("Couldn't create a network socket");
    goto neterr;
  }
  fcntl(game->net.sock_fd, F_SETFL, O_NONBLOCK);
  // disable Nagle's algo
  int one = 1;
  setsockopt(game->net.sock_fd, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
  game->net.spkinds.onblock = PCKT_END;
  game->net.spkinds.onerror = PCKT_INVALID;
  game->net.spkinds.ondisconnect = PCKT_DISCONNECT;

  struct sockaddr_in *saddr = &game->sdata.server_a;
  size_t addrl = sizeof (*saddr);

  if (game->sdata.server) {
    saddr->sin_addr.s_addr = INADDR_ANY;

    int retc = bind(game->net.sock_fd, (struct sockaddr *)saddr, addrl);
    if (retc < 0) {
      perror("Couldn't bind network socket");
      goto neterr;
    }
  
    setup_board_stacks(game);
    //almost_solved_stacks(game);
    
    if (listen(game->net.sock_fd, SOLR_MAX_PLAYERS) < 0) {
      perror("network socket listen");
      goto neterr;
    }
    game->srv.listen_sock_fd = game->net.sock_fd; // save for later switcheroos
    game->srv.listen_sock_pfd = (struct pollfd){
      .fd = game->srv.listen_sock_fd,
      .events = POLLIN,
    };
    
    printf("server running on port %d...\n", ntohs(sdata->server_a.sin_port));
  } else { // client
    int retv = connect(game->net.sock_fd, (struct sockaddr *)saddr, addrl);
    if (retv < 0) {
      if (errno == EINPROGRESS) {
        struct pollfd sock_pfd = {.fd = game->net.sock_fd, .events = POLLOUT};
        retv = poll(&sock_pfd, 1, 10*1000);
        if (retv == 0) {
          fprintf(stderr, "network connection timed out.\n");
          goto neterr;
        } else if (retv < 0) {
          perror("setup connection");
          goto neterr;
        }
        if (!(sock_pfd.revents & POLLOUT)) {
          fprintf(stderr, "socket still not writable after connection (?)\n");
          goto neterr;
        }
        int so_error_val = 0;
        socklen_t so_error_len = sizeof(int);
        if (getsockopt(
              game->net.sock_fd,
              SOL_SOCKET, SO_ERROR,
              &so_error_val, &so_error_len) < 0)
        {
          perror("getsockopt");
        }
        if (so_error_val != 0) {
          fprintf(stderr, "tcp connection failure: %s\n", strerror(so_error_val));
          goto neterr;
        }
      }
    }

    // announce our presence
    if (0) { // only needed for UDP
      *tx = (netpacket){0};
      tx->data_len = pack(tx->data, 1, MACROSTR(SOLR_NAME_LEN) "s", game->sdata.username);
      mahjcell_send(game, PCKT_JOIN);
    }

    int wait = 0;
    do {
      usleep(500*1e3);
      wait++;
      netmgr_sprecv(&game->net);
    } while(rx->kind != PCKT_ACK && wait < 50);
    if (wait >= 50) {
      fprintf(stderr, "couldn't log into lobby!\n");
      goto neterr;
    }

    unpack(rx->data, 1, "C", &game->client.uid);
    if (game->client.uid == 0xff) { fprintf(stderr, "game full!\n"); goto neterr; }
    game->st.players[game->client.uid].valid = 1;
    
    char ip4[INET_ADDRSTRLEN];
    printf("using server %s on port %d...\n",
           inet_ntop(AF_INET, &saddr->sin_addr, ip4, INET_ADDRSTRLEN),
           ntohs(sdata->server_a.sin_port));

    printf("loading graphics...\n");
    graphics_init(game);
    //gui_init(game);
  }
  game->st.stage = STAGE_PLAYING;
  
  return game_actual;
neterr:
  fprintf(stderr, "network error. quitting\n");
  game->st.stage = STAGE_NETERR;
  return game_actual;
}

void solr_post_reload(solr *_) {
  serde_init();
  printf("reloaded!\n");
}
void solr_pre_reload (solr *_) {
  printf("preparing to reload...\n");
}

void solr_cleanup(solr *game) {
  if (!game->sdata.server) {
    graphics_finish(game);
  }
  free(game->net.tx);
  free(game->net.rx);
  //gui_finish(game);
}

ssize_t server_sock_fd2uid(server *s, int fd) {
  ssize_t loc = -1;
  for (size_t i = 0; i < SOLR_MAX_PLAYERS; i++) {
    if (s->sock_fds[i] == fd) {
      loc = i;
      break;
    }
  }
  return loc;
}

ssize_t
true_uid(solr *s) {
  return server_sock_fd2uid(&s->srv, s->net.sock_fd);
}

float
lerp(float a, float b, float v) {
  return (a * (1.0 - v) + (b * v));
}

void
handle_pckt(solr *s, netpacket *p) {
  if (0 && p->kind != PCKT_POS) {
    char *pckt_str = netpacket_owned_debug_str(p, packet_kind_to_name);
    free(pckt_str);
  }
  pckt_handler_vec_base[p->kind](s, p); 
}

int
solr_fixed(solr *game, long step_ns) {
  player *my_player = game->st.players + game->client.uid;
  netpacket *tx = game->net.tx;
  netpacket *rx = game->net.rx; 
  game->st.tick += 1;

  if (game->sdata.server) {
    // server code
    int retv = poll(&game->srv.listen_sock_pfd, 1, 1);
    if (retv < 0) {
      perror("listen socket poll");
      fprintf(stderr, "ignoring and continuing\n");
    }
    if (!(game->srv.listen_sock_pfd.revents & POLLIN)) {
      goto skip_accept;
    }
    unsigned char vacant = 0xff;
    for (int i = 0; i < SOLR_MAX_PLAYERS; i++) {
      if (!game->st.players[i].valid) {
        vacant = i;
        break;
      }
    }
    if (vacant == 0xff) {
      fprintf(stderr, "got connection but game is full! ignoring.\n");
      goto skip_accept;
    }
    struct sockaddr_storage new_sock;
    socklen_t new_sock_len = sizeof(new_sock);
    int new_sock_fd = accept(
        game->srv.listen_sock_fd,
        (struct sockaddr *)&new_sock, &new_sock_len
      );
    if (new_sock_fd == -1) {
      perror("listen sock accept");
      fprintf(stderr, "ignoring and continuing\n");
      goto skip_accept;
    }
    fcntl(new_sock_fd, F_SETFL, O_NONBLOCK);
    game->srv.sock_addresses[vacant] = new_sock;
    game->srv.sock_fds[vacant] = new_sock_fd;
    game->srv.sock_last_comm[vacant] = time(NULL);

    netpacket virtual_join = {0};
      virtual_join.kind = PCKT_JOIN;
      virtual_join.data_len = pack(
          virtual_join.data, 1, MACROSTR(SOLR_NAME_LEN) "s", "anon");
    handle_pckt(game, &virtual_join);
skip_accept:

    if (game->st.stage == STAGE_FFW) {
        ffw_animate_server(game);
    }
    for (size_t pid = 0; pid < SOLR_MAX_PLAYERS; pid++) {
      if (!game->st.players[pid].valid)
        continue;

      // bind further comms to client sock
      game->net.sock_fd = game->srv.sock_fds[pid];
      
      // just to not leave unbounded
      for (size_t i = 0; i < 10000; i++) {
        const char *reject_reason;
        netmgr_sprecv(&game->net);

        bool should_break = rx->kind == PCKT_END || rx->kind == PCKT_DISCONNECT;
        if (should_break) break;
        if (rx->data_len > NETPACKET_MAX_PAYLOAD) {
          reject_reason = "packet too large:";
          goto reject_packet;
        }

        handle_pckt(game, rx);

        continue;
      reject_packet:
        fputs(reject_reason, stderr);
      }
    }
    if  (game->st.stage != STAGE_WON
      && game->st.tick % (30 * 5) == 0) {
      mahjcell_broadcast(game, PCKT_STDUMP);
    }
    goto end;
  } else {
    // client code
    // just to not leave unbounded
    for (size_t i = 0; i < 10000; i++) {
      const char *reject_reason;
      netmgr_sprecv(&game->net);

      bool should_break = rx->kind == PCKT_END || rx->kind == PCKT_DISCONNECT;
      if (should_break)
        break;
      if (rx->data_len > NETPACKET_MAX_PAYLOAD) {
        reject_reason = "packet too large:";
        goto reject_packet_client;
      }

      handle_pckt(game, rx);

      continue;
    reject_packet_client:
      fputs(reject_reason, stderr);
    }
  }

  if (game->st.stage == STAGE_WON) {
    game->st.players[SOLR_AUTO_PLAYER].valid = 0;
    if (flung_behaviour(game) == -1) {
      fling_suits(game);
    }
  }
  
  static ivec mouse_pos = {0};
  SDL_Event input;

  //nk_input_begin(game->nk);

  while (SDL_PollEvent(&input) > 0) {
    *game->net.tx = (netpacket){0};
    //game->net.peer = game->sdata.server_a;

    //nk_sdl_handle_event(&input);
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
      mahjcell_send(game, PCKT_LEAVE);
      goto leave;

    case SDL_MOUSEMOTION: {
        mouse_pos = (ivec){.x = input.motion.x, .y = input.motion.y};
        struct ivec pos = {.x = input.motion.x, .y = input.motion.y};
      	
        tx->data_len = pack(tx->data, 8, "QQC", &pos.x, &pos.y, &game->client.uid);
        mahjcell_send(game, PCKT_POS);
      }
      break;
    
    case SDL_MOUSEBUTTONUP: {
      player *myp = game->st.players + game->client.uid;
      if (input.button.button == SDL_BUTTON_RIGHT) {
        mahjcell_send(game, PCKT_CANCEL);
      }
      ssize_t stack_idx = stack_bbox(game, mouse_pos);
      if (stack_idx < 0) break;
      unsigned char n = game->st.board.stacks[stack_idx].top
          - card_bbox(game, stack_idx, mouse_pos);
      unsigned char n_held = myp->held.stack.top;

      move m = {
        .kind = myp->held.stack.top > 0 ? MOVE_PUSH : MOVE_TAKE,
        .num_cards = (n > n_held)? n : n_held,
      };
      m.stacks.src = m.kind == MOVE_PUSH ? STACK_PLAYER_BEGIN + game->client.uid : stack_idx;
      m.stacks.dst = m.kind == MOVE_TAKE ? STACK_PLAYER_BEGIN + game->client.uid : stack_idx;
      printf("sending move %s\n", move_repr(game, &m));

      if(m.stacks.dst == myp->held.camefrom){
        mahjcell_send(game, PCKT_CANCEL);
        break;
      }
      tx->data_len = pack(tx->data, 1, "[C]", &move_serde, &m);
      mahjcell_send(game, PCKT_MOVE);
      break;
    }
    
    case SDL_KEYUP:
      switch (input.key.keysym.sym) {
        case SDLK_RETURN:
        case SDLK_RETURN2:
        case SDLK_KP_ENTER:
          break;
        case SDLK_SPACE: {
          ssize_t stack_idx = stack_bbox(game, mouse_pos);
          if (stack_idx < 0) break;
          card_stack *dragon_stack = stack_from_id(game, stack_idx);
          if (dragon_stack->top <= 0) break;
          card *dragon = dragon_stack->s + dragon_stack->top - 1;
          if (dragon->r != RANK_DRAGON) break;
          move m = {
            .kind = MOVE_GROUP_DRAGONS,
            .suit = dragon->s,
            .num_cards = 1,
          };

          tx->data_len = pack(tx->data, 1, "[C]", &move_serde, &m);
          mahjcell_send(game, PCKT_MOVE);
          break;
        }
        case SDLK_r: {
          mahjcell_send(game, PCKT_RESTARTREQ);
          break;
        }
        case SDLK_f: {
          mahjcell_send(game, PCKT_FFWREQ);
          break;
        }
        case SDLK_d: {
          printf("state %d\n", game->st.stage);
          break;
        }
        case SDLK_h: {
          printf("reloading...\n");
          game->should_reload = 1;
          break;
        }
      }
      break;
    default:
      break;
    }
  }

  //nk_input_end(game->nk);
  
  end: return 1;
  leave:
  game->st.stage = STAGE_INVALID;
  return 0;
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

  rect.w = 64; rect.h = 64;
  
  //SDL_SetRenderDrawColor(rdr->rdr, 0xff, 0x20, 0x20, 0xb0);
  //SDL_RenderFillCircle(rdr->rdr, pos.x, pos.y, 13);

  if (!c->faceup) {
    atlas = &rdr->backs;
    rect = (SDL_Rect){.x = c->s * 64, .y = 0};
  }
  else {
    atlas = &rdr->suits;
    if (c->r == RANK_DRAGON) {
      rect.x = (4 + (3-c->s)) * 64;
      rect.y = 3 * 64;
    } else if (c->s == SUIT_FLOWER) {
      rect.x = 4*64;
      rect.y = 4*64;
    } else {
      rect.x = (c->r - 1) * 64;
      rect.y = (c->s - 1) * 64;
    }
  }
  if (SDL_RenderCopy(rdr->rdr, *atlas, &rect, &dst) < 0) {
    fprintf(stderr, "%s: %s", __func__, SDL_GetError());
    SDL_ClearError();
  }
}

void
card_stack_render(solr_rdr *rdr, card_stack *s, ivec pos) {
  SDL_Rect dst = posdims2rect(pos, rdr->card_dims);

  //SDL_SetRenderDrawColor(rdr->rdr, 0x00, 0xf0, 0x20, 0xb0);
  //SDL_RenderFillCircle(rdr->rdr, pos.x, pos.y, 13);

  if (s->top == 0) {
    SDL_Rect rect = {.x = 8*64, .y = 4*64, .w = 64, .h = 64};

    if (SDL_RenderCopy(rdr->rdr, rdr->suits, &rect, &dst) < 0) {
      fprintf(stderr, "%s: %s", __func__, SDL_GetError());
      SDL_ClearError();
    }
    return;
  }
  if (s->pile) {
    SDL_Rect rect = {.x = 0, .y = 0, .w = 88, .h = 124};
    for (ssize_t i = 0; i < s->top; i++) {
      if (i == s->top - 1) { card_render(rdr, &s->s[i], (ivec){dst.x,dst.y}); }
      else {
	continue;
	// FIXME: impl this properly
	if (SDL_RenderCopy(rdr->rdr, rdr->backs, &rect, &dst) < 0) {
        fprintf(stderr, "%s: %s", __func__, SDL_GetError());
        SDL_ClearError();
	}
      }
      dst.y -= 2;
    }
  }

  else if (!s->splayed) {
    card_render(rdr, &s->s[s->top - 1], pos);
  } else {
    for (size_t i = 0; i < s->top; i++) {
      card_render(rdr, &s->s[i], pos);
      pos.y += rdr->card_dims.y/4;
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
solr_render (solr *game, double a) {
  solr_rdr *r = &game->rdr;

  SDL_SetRenderDrawColor(r->rdr, 0x3c, 0x3c, 0x3c, SDL_ALPHA_OPAQUE);
  if (game->st.stage != STAGE_WON) {
    SDL_RenderClear(r->rdr);
  } else {
    fvec cardpos = game->st.flung_card.pb.pos;
    card_render(r, &game->st.flung_card.c, (ivec){.x = cardpos.x, .y = cardpos.y});
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

  //gui_update(game);
  // triggers the double buffers for multiple rendering
  end: SDL_RenderPresent(r->rdr);
}

// TODO: stack randomization
// TODO: disconnect on win (or do something better)

