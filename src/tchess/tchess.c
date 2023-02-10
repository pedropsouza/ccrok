#include "tchess.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static void
serde_init() {
  serde_fun *t = serializer_table;
  t['b'] = board_serde;
  t['c'] = card_serde;
  t['f'] = card_stack_serde;
  t['m'] = move_serde;
  t['p'] = player_serde;
  t['G'] = gamestate_serde;
}

size_t
card_serde(pbuf_t *buf, char mode, void *vc) {
  card *c = vc;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(card);
  return pack_unpack(buf, mode, 1, "CCCC", &c->s, &c->r, &c->faceup, &c->clr);
}

size_t
card_stack_serde(pbuf_t *buf, char mode, void *vs) {
  card_stack *s = vs;
  // TODO: VARIABLE LEN
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(card_stack);
  return pack_unpack(buf, mode, 8, "QQ52cCCCC", &s->dims.x, &s->dims.y,
    &s->s, &s->top, &s->splayed, &s->pile, &s->id);
}

size_t
move_serde(pbuf_t *buf, char mode, void *vm) {
  move *m = vm;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(move);
  return pack_unpack(buf, mode, 1, "CCCC", &m->kind, &m->stacks.src, &m->stacks.dst, &m->num_cards);
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
  return pack_unpack(buf, mode, 8, "QQ" MACROSTR(TCHESS_NAME_LEN) "sfCC",
    &p->pos.x, &p->pos.y, &p->username, &p->held.stack, &p->held.camefrom, &p->valid);
}

size_t
gamestate_serde(pbuf_t *buf, char mode, void *vst) {
  gamestate *st = vst;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(gamestate);
  return pack_unpack(buf, mode, 8, "Cb" MACROSTR(TCHESS_MAX_PLAYERS) "pQ",
    &st->stage, &st->board, &st->players, &st->tick);
}

SDL_Rect
posdims2rect(ivec pos, ivec dims) {
  return (SDL_Rect){pos.x, pos.y, dims.x, dims.y};
}

static ivec stack_areas[STACK_BOARD_MAX] = {
    [0]  = {.x = 12,                 .y = 64,     },
    [1]  = {.x = 12 + 1 * (12 + 88), .y = 140 + 64},
    [2]  = {.x = 12 + 2 * (12 + 88), .y = 140 + 64},
    [3]  = {.x = 12 + 3 * (12 + 88), .y = 140 + 64},
    [4]  = {.x = 12 + 4 * (12 + 88), .y = 140 + 64},
    [5]  = {.x = 12 + 5 * (12 + 88), .y = 140 + 64},
    [6]  = {.x = 12 + 6 * (12 + 88), .y = 140 + 64},
    [7]  = {.x = 12 + 7 * (12 + 88), .y = 140 + 64},
    [8]  = {.x = 12 + 4 * (12 + 88), .y = 64,     },
    [9]  = {.x = 12 + 5 * (12 + 88), .y = 64,     },
    [10] = {.x = 12 + 6 * (12 + 88), .y = 64,     },
    [11] = {.x = 12 + 7 * (12 + 88), .y = 64,     },
    [12] = {.x = 12 + 1 * (12 + 88), .y = 64,     },
};
ivec card_dims = {.x = 88, .y = 124};

static void
update_stack_dims(card_stack *s) {
  s->dims.x = 88;
  s->dims.y = 124;
  if (s->splayed) {
    s->dims.y += 20*s->top;
  } else {
    s->dims.y += (16*s->pile*(s->top > 0));
  }
}

void
shuffle_stack(card_stack *s) {
  if (s->top > 1) {
    size_t i;
    for (i = 0; i < s->top - 1; i++) {
      size_t j = i + rand() / (RAND_MAX / (s->top - i) + 1);
      card t = s->s[j];
      s->s[j] = s->s[i];
      s->s[i] = t;
    }
  }
}

void
push_card_stack(card_stack *dst, card_stack *src) {
  assert(dst != src);
  assert(dst->top + src->top <= 13*4);
  assert(src->top > 0);
  for (ssize_t i = 0; i < src->top; i++) {
    dst->s[dst->top] = src->s[i];
    src->s[i] = (card){0};
    dst->top++;
  }
  src->top = 0;
  update_stack_dims(dst);
  update_stack_dims(src);
}
void
take_card_stack(card_stack *src, card_stack *dst, size_t n) {
  assert(dst != src);
  assert(src->top - (ssize_t)n >= 0);
  assert(dst->top + n <= 13*4);

  for (ssize_t i = src->top - n; i < src->top; i++) {
    card c = src->s[i];
    src->s[i] = (card){0};
    dst->s[dst->top] = c;
    dst->top++;
  }
  src->top -= n;
  update_stack_dims(dst);
  update_stack_dims(src);
}
static void
expose_top_card_stack(card_stack *s) {
  if (s->top == 0) return;
  s->s[s->top - 1].faceup = 1;
}

static card_stack*
stack_from_id(tchess *s, uint8_t id) {
  return id >= STACK_PLAYER_BEGIN ? &s->st.players[id - STACK_PLAYER_BEGIN].held.stack :
    s->st.board.stacks + id;
}

static void
fling_suits(tchess *s) {
  if (s->st.flung_card.valid) return;
  uint8_t rsid = (rand() % (STACK_SUITS_END - STACK_SUITS_BEGIN + 1)) + STACK_SUITS_BEGIN;
  card_stack *fs = stack_from_id(s, rsid);
  if (fs->top == 0) return;
  card c = fs->s[fs->top - 1];
  fs->s[fs->top - 1] = (card){0};
  fs->top--;

  physical_card pc = {
    .pb.pos = stack_areas[fs->id],
    .pb.vel = (ivec){.x = rand() % 20 - 10, .y = 0},
    .pb.acc = ivec_zero(),
    .c = c,
    .valid = 1,
  };
  s->st.flung_card = pc;
}

static int flung_behaviour(tchess *s) {
  physical_card *pc = &s->st.flung_card;
  if (!pc->valid) return -1;
  int64_t x = pc->pb.pos.x;
  int64_t y = pc->pb.pos.y;
  if(x < -card_dims.x || x > s->sdata.window_w
    || (y == s->sdata.window_h - card_dims.y && pc->pb.vel.y < 2)
  ) {
    s->st.flung_card.valid = 0;
    return -1;
  }

  physbody_accel(&pc->pb, (ivec){0,1});
  physbody_tick(&pc->pb);
  if (y + card_dims.y > s->sdata.window_h) {
    physbody_accel(&pc->pb, (ivec){0, -pc->pb.vel.y - pc->pb.vel.y / 2});
    pc->pb.pos.y = s->sdata.window_h - card_dims.y;
  }
  return 0;
}

static SDL_Texture *
tex_from_file(SDL_Renderer *rdr, const char* filename) {
  FILE *f = fopen(filename, "r");
  int w,h,chnum;
  stbi_uc *pix = stbi_load_from_file(f, &w, &h, &chnum, 4);
  for (int i = 0; i < w*h; i++) {
    stbi_uc *head = pix + i * 4;
    stbi_uc *r = head, *g = head + 1, *b = head + 2, *a = head + 3;
    if (*g == 128 && *b == 128) { *a = 0; }
  }
  printf("%s, %dx%d, chn %d\n", filename, w, h, chnum);
  SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(pix, w, h, 32, 4*w, 0x0000ff, 0x00ff00, 0xff0000, 0xff000000);
  SDL_Texture *tex = SDL_CreateTextureFromSurface(rdr, surf);
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
  SDL_FreeSurface(surf);
  stbi_image_free(pix);
  fclose(f);
  return tex;
}

static ssize_t
stack_bbox(tchess *s, ivec mpos) {
  for (size_t i = 0; i < STACK_BOARD_MAX; i++) {
    SDL_Rect box = posdims2rect(stack_areas[i], s->st.board.stacks[i].dims);
    SDL_Point p = {.x = mpos.x, .y = mpos.y};
    if (SDL_PointInRect(&p, &box) == SDL_TRUE) {
      return i;
    }
  }
  return -1;
}

static ssize_t
card_bbox(tchess *s, size_t stack_idx, ivec mpos) {
  assert(stack_idx < STACK_BOARD_MAX);
  ivec *area = stack_areas + stack_idx;
  card_stack *ss = s->st.board.stacks + stack_idx;
  if (ss->top == 0) goto not_found;

  int ystep = (ss->dims.y - card_dims.y)/(ss->top - 1 * (ss->top > 1));
  ivec cpos = {.x = area->x, .y = area->y + ystep * (ss->top - 1)};
  for (ssize_t i = ss->top - 1; i >= (ss->splayed ? 0 : ss->top - 1); i--) {
    SDL_Rect box = posdims2rect(cpos, card_dims);
    SDL_Point p = {.x = mpos.x, .y = mpos.y};
    if (SDL_PointInRect(&p, &box) == SDL_TRUE) {
      return i;
    }
    cpos.y -= ystep;
  }

  // returning zero is technically incorrect,
  // as it is a valid index. but it's easier this way
  not_found: return 0;
}

int
move_valid(tchess *s, move *m, char **reason) {
  *reason = "valid";
  if (m->stacks.src > STACK_MAX || m->stacks.dst > STACK_MAX) {
    *reason = "stack oob";
    return 0;
  }
  card_stack *src = stack_from_id(s, m->stacks.src);
  card_stack *dst = stack_from_id(s, m->stacks.dst);

#define IS_TABLEAU(x) (x >= STACK_TABLEAU_BEGIN && x <= STACK_TABLEAU_END)
#define IS_STOCK(x) (x == STACK_STOCK)
#define IS_WASTE(x) (x == STACK_WASTE)
#define IS_SUITS(x) (x >= STACK_SUITS_BEGIN && x <= STACK_SUITS_END)

  if (m->kind == MOVE_TAKE) {
    if (dst->top > 0) {
      *reason = "taking with nonempty dst";
      return 0;
    }
    if (((ssize_t)src->top) - m->num_cards < 0) {
      *reason = "not enough cards to take";
      return 0;
    }
    if (IS_STOCK(src->id) || IS_SUITS(src->id) || IS_WASTE(src->id))
      return 1;
    if (IS_TABLEAU(src->id)) {
      ssize_t idx = src->top - m->num_cards;
      assert(idx < 4*13 && idx >= 0);
      int r = src->s[idx].faceup;
      if (!r) *reason = "taking an unknown card";
      return r;
    }
  }
  else if (m->kind == MOVE_PUSH) {
    int empty_dst = dst->top == 0;
    card *top_dst = NULL;
    if (!empty_dst) top_dst = &dst->s[dst->top - 1];
    card *bot_src = &src->s[0];
    if (src->top == 0) {
      *reason = "no cards to push";
      return 0;
    }
    if (((ssize_t)dst->top) + m->num_cards > 13*4) {
      *reason = "too many cards";
      return 0;
    }
    if (IS_TABLEAU(dst->id)) {
      if (empty_dst) {
        if (bot_src->r != RANK_K) {
          *reason = "only kings can be placed there";
          return 0;
        }
        return 1;
      }
      bool color = top_dst->clr != bot_src->clr;
      bool rank = top_dst->r == bot_src->r + 1;
      int r = color && rank;
      if (!r) *reason = "against the rules";
      return r;
    }
    if (IS_STOCK(dst->id)) {
      *reason = "push to stock is forbidden";
      return 0;
    }
    if (IS_WASTE(dst->id)) {
      *reason = "push to waste is forbidden";
      return 0;
    }
    if (IS_SUITS(dst->id)) {
      if (empty_dst) {
        if (bot_src->r != RANK_A) {
          *reason = "only aces can be placed there";
          return 0;
        }
        return 1;
      }
      bool number = src->top == 1;
      bool color = top_dst->clr == bot_src->clr;
      bool rank = bot_src->r == top_dst->r + 1;
      bool suit = bot_src->s == top_dst->s;
      int r = number && color && rank && suit;
      if (!r) *reason = "against the rules";
      return r;
    }
  }
  *reason = "invalid move";
  return 0;

#undef IS_TABLEAU
#undef IS_STOCK
#undef IS_SUITS
}

static void
graphics_init(tchess *s) {
  tchess_rdr *rdat = &s->rdr;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  rdat->win = SDL_CreateWindow("tchess", SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED, s->sdata.window_w, s->sdata.window_h, 0);
  rdat->rdr = SDL_CreateRenderer(rdat->win, -1, SDL_RENDERER_ACCELERATED);
  if (rdat->rdr == NULL) {
    fprintf(stderr, "failed to create SDL renderer: %s\n", SDL_GetError());
    return; // FIXME: add proper error handling up the callstack
  }

  SDL_RendererInfo rdr_info;
  printf("available renderers:\n");
  for (int i = 0; i < SDL_GetNumRenderDrivers(); i++) {
    SDL_GetRenderDriverInfo(i, &rdr_info);
    printf("\t%d: %s\n", i, rdr_info.name);
  }
  SDL_GetRendererInfo(rdat->rdr, &rdr_info);
  printf("renderer: %s\n", rdr_info.name);

  SDL_SetRenderDrawBlendMode(rdat->rdr, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(rdat->rdr, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(rdat->rdr);

  // modified dimensions maybe
  SDL_GetRendererOutputSize(rdat->rdr, (int*)&s->sdata.window_w, (int*)&s->sdata.window_h);

  int smaller = (s->sdata.window_w > s->sdata.window_h) ?
    s->sdata.window_h : s->sdata.window_w;

  rdat->backs         = tex_from_file(rdat->rdr, "resources/cards/Card_Back-88x124.png");
  rdat->slot          = tex_from_file(rdat->rdr, "resources/cards/Card_Slot-88x124.png");
  rdat->piles         = tex_from_file(rdat->rdr, "resources/cards/Card_DeckA-88x140.png");
  rdat->suits[SUIT_H] = tex_from_file(rdat->rdr, "resources/cards/Hearts-88x124.png");
  rdat->suits[SUIT_D] = tex_from_file(rdat->rdr, "resources/cards/Diamonds-88x124.png");
  rdat->suits[SUIT_C] = tex_from_file(rdat->rdr, "resources/cards/Clubs-88x124.png");
  rdat->suits[SUIT_S] = tex_from_file(rdat->rdr, "resources/cards/Spades-88x124.png");
}

void
graphics_finish(tchess *s) {
  tchess_rdr *rdat = &s->rdr;
  SDL_DestroyTexture(rdat->backs);
  SDL_DestroyTexture(rdat->piles);
  SDL_DestroyTexture(rdat->slot);
  for (size_t i = SUIT_BEG; i < SUIT_MAX; i++) {
    SDL_DestroyTexture(rdat->suits[i]);
  }
  SDL_DestroyRenderer(rdat->rdr);
  SDL_DestroyWindow(rdat->win);
  SDL_Quit();
}

static void
setup_board_stacks(tchess *game) {
  for (size_t i = 0; i < STACK_BOARD_MAX; i++) {
    game->st.board.stacks[i].dims = card_dims;
  }
  for (size_t i = STACK_TABLEAU_BEGIN; i <= STACK_TABLEAU_END; i++) {
    game->st.board.stacks[i].splayed = 1;
  }
  for (size_t i = 0; i < TCHESS_MAX_PLAYERS; i++) {
    game->st.players[i].held.stack.splayed = 1;
  }

  card_stack *stock = stack_from_id(game, STACK_STOCK);
  card_stack *waste = stack_from_id(game, STACK_WASTE);
  stock->pile = 1;
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
    for (int j = 0; j < i; j++) {
      take_card_stack(stock, col, 1);
    }
    expose_top_card_stack(col);
  }

  for (size_t i = 0; i < STACK_MAX; i++) {
    card_stack *s = stack_from_id(game, i);
    update_stack_dims(s);
    s->id = i;
  }
}

static void almost_solved_stacks(tchess *game) {
  for (size_t i = 0; i < STACK_BOARD_MAX; i++) {
    game->st.board.stacks[i].dims = card_dims;
  }
  for (size_t i = STACK_TABLEAU_BEGIN; i <= STACK_TABLEAU_END; i++) {
    game->st.board.stacks[i].splayed = 1;
  }
  for (size_t i = 0; i < TCHESS_MAX_PLAYERS; i++) {
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
    update_stack_dims(s);
    s->id = i;
  }
}

static int
is_solved(tchess *s) {
  size_t c_suits = 0;
  for (size_t i = STACK_SUITS_BEGIN; i <= STACK_SUITS_END; i++) {
    card_stack *cs = s->st.board.stacks + i;
    c_suits += cs->top;
  }
  printf("%zu in suits\n", c_suits);
  return c_suits >= 52;
}

sch_trans
tchess_setup(scheduler *s) {
  setup_data *sdata = (setup_data *)s->setup_data;
  s->persistent_data =
      safe_calloc(sizeof (tchess), "setup persistent_data alloc");
  tchess *game = (tchess *)s->persistent_data;

  serde_init();
  srand(time(NULL));

  game->sdata = *sdata;

  game->net.sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (game->net.sock_fd == -1) {
    perror("Couldn't create a network socket");
    goto neterr;
  }
  fcntl(game->net.sock_fd, F_SETFL, O_NONBLOCK);
  game->net.spkinds.onblock = PCKT_END;
  game->net.spkinds.onerror = PKCT_INVALID;

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
    game->net.tx.data_len = pack(game->net.tx.data, 1, MACROSTR(TCHESS_NAME_LEN) "s", game->sdata.username);
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
    
    char ip4[INET_ADDRSTRLEN];
    printf("using server %s on port %d...\n",
           inet_ntop(AF_INET, &saddr->sin_addr, ip4, INET_ADDRSTRLEN),
           ntohs(sdata->server_a.sin_port));

    printf("loading graphics...\n");
    graphics_init(game);
  }

  
   
  return (sch_trans){.s = SCHED_CONTINUE, .why = "setup finished"};
neterr:
  return (sch_trans){.s = SCHED_ERR, .why = "network setup error"};
}

sch_trans
tchess_cleanup(scheduler *s) {
  tchess *game = (tchess*)s->persistent_data;
  if (!game->sdata.server) {
    graphics_finish(game);
  }
  free(s->persistent_data);
  s->persistent_data = NULL;

  return (sch_trans){.s = SCHED_HALT, .why = "cleanup finished"};
}

ssize_t server_sockaddr2uid(server *s, struct sockaddr_in *sock) {
  ssize_t loc = -1;
  for (size_t i = 0; i < TCHESS_MAX_PLAYERS; i++) {
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
true_uid(tchess *s) {
  return server_sockaddr2uid(&s->srv, &s->net.peer);
}

static void
broadcast(tchess *s) {
  for (int i = 0; i < TCHESS_MAX_PLAYERS; i++) {
    if (!s->st.players[i].valid) continue;
    s->net.peer = s->srv.socks[i];
    netmgr_send(&s->net);
  }
}

static void
handle_pckt(tchess *s, netpacket *p) {
  int8_t uid;
  if(p->kind != PCKT_POS) printf("got a packet kinded %d\n", p->kind);
  switch (p->kind) {
    case PCKT_JOIN: {
      if (!s->sdata.server) return;
      unsigned char vacant = 0xff;
      for (int i = 0; i < TCHESS_MAX_PLAYERS; i++) {
        if (!s->st.players[i].valid) {
          vacant = i;
          break;
        }
      }
      if (vacant == 0xff) {} // TODO

      s->st.players[vacant].valid = 1;
      s->st.players[vacant].pos = (ivec){0};
        
      s->srv.socks[vacant] = s->net.peer;
        
      if (p->data_len > 4 && p->data_len < TCHESS_NAME_LEN + 4) {
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
    printf("move %d %d %d %d\n", m.kind, m.stacks.src, m.stacks.dst, m.num_cards);
    if (!move_valid(s, &m, &rej_reason)) {
      printf("invalid move: %s\n", rej_reason);
      break;
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
        if (cf->top > 0) {
          expose_top_card_stack(cf);
        }
      }
    } else {
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
            printf("set camefrom %d\n", src->id);
            s->st.players[m.stacks.dst - STACK_PLAYER_BEGIN].held.camefrom = src->id;
          }
        }
      } else {
        take_card_stack(src, dst, m.num_cards);
        if (m.stacks.dst >= STACK_PLAYER_BEGIN) {
          printf("set camefrom %d\n", src->id);
          s->st.players[m.stacks.dst - STACK_PLAYER_BEGIN].held.camefrom = src->id;
        }
      }
    }
    
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
      push_card_stack(stack_from_id(s, pl->held.camefrom), &pl->held.stack);
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
    unpack(s->net.rx.data, 8, "QQC", &pos.x, &pos.y, (uint8_t*)&uid);
    if (s->sdata.server) uid = true_uid(s);
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
    if (s->st.stage == STAGE_WON) return;
    gamestate nst = {0};
    printf("received statedump\n");
    unpack(p->data, 8, "G", &nst);
    s->st = nst;
    break;
  }
  default: break;
  }
}

sch_trans
tchess_fixed(scheduler *s, long step_ns) {
  tchess *game = (tchess *)s->persistent_data;
  player *my_player = game->st.players + game->client.uid;
  game->st.tick += 1;

  while(1) {
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

  if (game->sdata.server && game->st.tick % 200 == 0) {
    game->net.tx.kind = PCKT_STDUMP;
    game->net.tx.data_len = pack(game->net.tx.data, 8, "G", &game->st);
    broadcast(game);
  }
  
  if (game->st.stage == STAGE_WON) {
    if (flung_behaviour(game) == -1) {
      fling_suits(game);
    }
  }
  
  if (game->sdata.server) goto end;
  
  static ivec mouse_pos = {0};
  SDL_Event input;
  while (SDL_PollEvent(&input) > 0) {
    switch (input.type) {
    case SDL_QUIT:
      printf("leaving...\n");
      game->net.tx = (netpacket){0};
      game->net.tx.kind = PCKT_LEAVE;
      game->net.peer = game->sdata.server_a;
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
        game->net.tx = (netpacket){0};
        game->net.tx.kind = PCKT_CANCEL;
        game->net.tx.data_len = pack(game->net.tx.data, 1, "C", &game->client.uid);
        game->net.peer = game->sdata.server_a;
        netmgr_send(&game->net);
        break;
      }
      ssize_t stack_idx = stack_bbox(game, mouse_pos);
      if (stack_idx < 0) break;
      unsigned char n = game->st.board.stacks[stack_idx].top
          - card_bbox(game, stack_idx, mouse_pos);
      printf("bbox %zd %u @ %ld,%ld\n", stack_idx, n, mouse_pos.x, mouse_pos.y);

      if (stack_idx < 0) break;
      move m = {
        .kind = myp->held.stack.top > 0 ? MOVE_PUSH : MOVE_TAKE,
        .num_cards = n,
      };
      m.stacks.src = m.kind == MOVE_PUSH ? STACK_PLAYER_BEGIN + game->client.uid : stack_idx;
      m.stacks.dst = m.kind == MOVE_TAKE ? STACK_PLAYER_BEGIN + game->client.uid : stack_idx;
      
      game->net.tx = (netpacket){0};
      game->net.tx.kind = PCKT_MOVE;
      game->net.tx.data_len = pack(game->net.tx.data, 1, "m", &m);
      game->net.peer = game->sdata.server_a;
      netmgr_send(&game->net);
      break;
    }
    /*

      static ssize_t
      card_bbotchess *s* size_t stack_idx,s, ivemc pos) {

  SDL_Rect *area = stack_areas + stack_idx;
      card_stack *ss = s->st.board.stacks + stack_idx;
      ivec cpos = {.x = area->x, .y = area->y};
    cpos.y += (ss->dims.y - card_dims.y);
        for (size_t ssss->top - 1; i >= ()->splayed ? 00: ss->top - 1 )=op--i++) {
      cpos.y -= (ss->dims.y - card_dims.y) / ss->top;
        SDL_Rect box = posdims2rect(cpos, card_dims);
      SDL_Point p = {.x = mpos.x, .y = mpos.y};
      if (SDL_PointInRect(&p, &box) == SDL_TRUE) {
        return i;
      }
      }
      return -1;
      }
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
  leave:
  return (sch_trans){.s = SCHED_CLEANUP, .why = "user requested quit"};
}

void
player_render(tchess_rdr *rdr, player *p) {
  SDL_SetRenderDrawColor(rdr->rdr, 0xff, 0x20, 0x20, 0xb0);
  SDL_RenderFillCircle(rdr->rdr, p->pos.x, p->pos.y, 3);
}

void
card_render(tchess_rdr *rdr, card* c, ivec pos) {
  SDL_Rect rect;
  SDL_Rect dst = posdims2rect(pos, card_dims);
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
card_stack_render(tchess_rdr *rdr, card_stack *s, ivec pos) {
  SDL_Rect dst = posdims2rect(pos, (ivec){88, 124});
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
    //for (ssize_t i = s->top - 1; i >= 0; i--) {
    for (ssize_t i = i; i < s->top; i++) {
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
      pos.y += (s->dims.y - card_dims.y)/(s->top - 1 * (s->top > 1));
    }
  }
}

void
tchess_render(scheduler *s, double a) {
  tchess *game = (tchess *)s->persistent_data;
  tchess_rdr *r = &game->rdr;
  

  SDL_SetRenderDrawColor(r->rdr, 0x3c, 0x3c, 0x3c, SDL_ALPHA_OPAQUE);
  if (game->st.stage != STAGE_WON) {
    SDL_RenderClear(r->rdr);
  } else {
    card_render(r, &game->st.flung_card.c, game->st.flung_card.pb.pos);
    goto end;
  }

  for (size_t i = 0; i < STACK_BOARD_MAX; i++) {
    card_stack_render(r, game->st.board.stacks + i, (ivec){stack_areas[i].x, stack_areas[i].y});
  }

  for (size_t i = 0; i < TCHESS_MAX_PLAYERS; i++) {
    player *p = game->st.players + i;
    if (p->valid) {
      if (p->held.stack.top > 0){
        ivec offset = ivec_sub(p->pos, ivec_div(card_dims, (ivec){2,2}));
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

