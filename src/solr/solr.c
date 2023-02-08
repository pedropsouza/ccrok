#include "solr.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


SDL_Rect posdims2rect(ivec pos, ivec dims) {
  return (SDL_Rect){pos.x, pos.y, dims.x, dims.y};
}

static SDL_Rect stack_areas[STACK_MAX] = {
    [0]  = {.x = 12,            .y = 12,       .w = 88, .h = 124},
    [1]  = {.x = 1 * (12 + 88), .y = 140 + 12, .w = 88, .h = 124},
    [2]  = {.x = 2 * (12 + 88), .y = 140 + 12, .w = 88, .h = 124},
    [3]  = {.x = 3 * (12 + 88), .y = 140 + 12, .w = 88, .h = 124},
    [4]  = {.x = 4 * (12 + 88), .y = 140 + 12, .w = 88, .h = 124},
    [5]  = {.x = 5 * (12 + 88), .y = 140 + 12, .w = 88, .h = 124},
    [6]  = {.x = 6 * (12 + 88), .y = 140 + 12, .w = 88, .h = 124},
    [7]  = {.x = 7 * (12 + 88), .y = 140 + 12, .w = 88, .h = 124},
    [8]  = {.x = 4 * (12 + 88), .y = 12,       .w = 88, .h = 124},
    [9]  = {.x = 5 * (12 + 88), .y = 12,       .w = 88, .h = 124},
    [10] = {.x = 6 * (12 + 88), .y = 12,       .w = 88, .h = 124},
    [11] = {.x = 7 * (12 + 88), .y = 12,       .w = 88, .h = 124},
    [12] = {.x = 7 * (12 + 88), .y = 12,       .w = 88, .h = 124},
    [13] = {.x = 2 * (12 + 88), .y = 12,       .w = 88, .h = 124},
};

static void
update_stack_dims(card_stack *s) {
  s->dims.x = 88;
  s->dims.y = 124;
  if (s->splayed) {
    s->dims.y += 12*s->top;
  } else {
    s->dims.y += (16*s->pile*(s->top > 0));
  }
}

void
push_card_stack(card_stack *dst, card_stack *src) {
  assert(dst->top + src->top <= 13*4);
  assert(src->top > 0);
  for (ssize_t i = src->top - 1; i >= 0; i--) {
    dst->s[dst->top] = src->s[i];
    dst->top++;
  }
  src->top = 0;
  update_stack_dims(dst);
  update_stack_dims(src);
}
void
pop_card_stack(card_stack *src, card_stack *dst, size_t n) {
  assert((ssize_t)src->top - n >= 0);
  assert(dst->top + n <= 13*4);

  for (ssize_t i = 0; i < n; i++) {
    card c = src->s[src->top - 1];
    src->s[src->top - 1] = (card){0};
    dst->s[dst->top] = c;
    src->top--;
    dst->top++;
  }
  update_stack_dims(dst);
  update_stack_dims(src);
}
static void
expose_top_card_stack(card_stack *s) {
  if (!s->top) return;
  s->s[s->top - 1].faceup = 1;
}

static SDL_Texture *
tex_from_file(SDL_Renderer *rdr, const char* filename) {
  FILE *f = fopen(filename, "r");
  int w,h,chnum;
  stbi_uc *pix = stbi_load_from_file(f, &w, &h, &chnum, 3);
  printf("%s, %dx%d, chn %d\n", filename, w, h, chnum);
  SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(pix, w, h, 24, 3*w, 0x0000ff, 0x00ff00, 0xff0000, 0);
  SDL_Texture *tex = SDL_CreateTextureFromSurface(rdr, surf);
  SDL_FreeSurface(surf);
  stbi_image_free(pix);
  fclose(f);
  return tex;
}

static ssize_t bbox_check(solr *s, ivec pos) {
  for (size_t i = 0; i < STACK_MAX; i++) {
    SDL_Rect *r = stack_areas + i;
    SDL_Point p = {.x=pos.x, .y=pos.y};
    if (SDL_PointInRect(&p, r) == SDL_TRUE) {
      return i;
    }
  }
  return -1;
}

static void
graphics_init(solr *s) {
  solr_rdr *rdat = &s->rdr;

  SDL_Init(SDL_INIT_VIDEO);
  rdat->win = SDL_CreateWindow("solr", SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED, s->sdata.window_w, s->sdata.window_h, 0);
  rdat->rdr = SDL_CreateRenderer(rdat->win, -1, SDL_RENDERER_ACCELERATED);
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
graphics_finish(solr *s) {
  solr_rdr *rdat = &s->rdr;
  SDL_DestroyTexture(rdat->backs);
  SDL_DestroyTexture(rdat->piles);
  for (size_t i = SUIT_BEG; i < SUIT_MAX; i++) {
    SDL_DestroyTexture(rdat->suits[i]);
  }
  SDL_DestroyRenderer(rdat->rdr);
  SDL_DestroyWindow(rdat->win);
  SDL_Quit();
}

sch_trans
solr_setup(scheduler *s) {
  setup_data *sdata = (setup_data *)s->setup_data;
  s->persistent_data =
      safe_calloc(sizeof (solr), "setup persistent_data alloc");
  solr *game = (solr *)s->persistent_data;


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

    printf("server running on port %d...\n", SOLR_PORT);
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
    
    char ip4[INET_ADDRSTRLEN];
    printf("using server %s on port %d...\n",
           inet_ntop(AF_INET, &saddr->sin_addr, ip4, INET_ADDRSTRLEN),
           SOLR_PORT);

    printf("loading graphics...\n");
    graphics_init(game);
  }

  for (size_t i = 0; i < STACK_MAX; i++) {
    ivec dims = {stack_areas[i].w, stack_areas[i].h};
    game->st.board.stacks[i].dims = dims;
  }
  for (size_t i = STACK_TABLEAU_BEGIN; i <= STACK_TABLEAU_END; i++) {
    game->st.board.stacks[i].splayed = 1;
  }
  for (size_t i = 0; i < SOLR_MAX_PLAYERS; i++) {
    update_stack_dims(&game->st.players[i].held.stack);
  }

  for (int suit = SUIT_BEG; suit < SUIT_MAX; suit++) {
    int clr = suit <= 2;
    for (int rank = RANK_BEG; rank < RANK_MAX; rank++) {
      card_stack *stack = &game->st.board.stacks[0];
      stack->s[stack->top] = (card){.s = suit, .r = rank, .faceup = 0, .clr = clr};
      stack->top++;
    }
  }
  update_stack_dims(&game->st.board.stacks[0]);
   
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
    if (s->socks[i].sin_addr.s_addr == sock->sin_addr.s_addr) {
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

static void
broadcast(solr *s) {
  for (int i = 0; i < SOLR_MAX_PLAYERS; i++) {
    if (!s->st.players[i].valid) continue;
    s->net.peer = s->srv.socks[i];
    netmgr_send(&s->net);
  }
}

static void
handle_pckt(solr *s, netpacket *p) {
  if(p->kind != PCKT_POS) printf("got a packet kinded %d\n", p->kind);
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

      s->st.players[vacant] =
          (player){.valid = 1, .pos = {0}, .username = "anon", .held = {0}};
        
      s->srv.socks[vacant] = s->net.peer;
        
      if (p->data_len > 4 && p->data_len < SOLR_NAME_LEN + 4) {
        unpack(p->data, 1, "s", &s->st.players[vacant].username);
      }

      s->net.tx.kind = PCKT_ACK;
      s->net.tx.data_len = pack(s->net.tx.data, 1, "C", &vacant);
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
    card_stack *stacks = s->st.board.stacks;
    unsigned char uid;
    unsigned char stack_idx;
    char n = 0;
    unpack(s->net.rx.data, 1, "CCC", &uid, &stack_idx, &n);
    if (s->sdata.server) uid = true_uid(s);

    // FIXME: proper handling
    assert(stack_idx < STACK_MAX);
    card_stack *subj_s = stacks + stack_idx;
    player *subj_p = s->st.players + uid;


    if (p->kind == PCKT_PUSH) {
      if (subj_p->held.stack.top == 0) return; // should be popping
      if (((ssize_t)subj_s->top) + n > 13*4) return; // would overrun buffer
      push_card_stack(s->st.board.stacks + stack_idx, &s->st.players[uid].held.stack);
      subj_p->held = (held_stack){0};
    } else {
      if (subj_p->held.stack.top > 0) return; // should be pushing
      if (((ssize_t)subj_s->top) - n < 0) return; // there is nothing to pop
      if (stack_idx == STACK_STOCK) {
        expose_top_card_stack(subj_s);
        pop_card_stack(subj_s, s->st.board.stacks + STACK_WASTE, 1);
      } else {
        pop_card_stack(subj_s, &subj_p->held.stack, n);
        subj_p->held.camefrom = stack_idx;
      }
    }
    
    if (s->sdata.server) {
      s->net.tx.kind = p->kind;
      s->net.tx.data_len = pack(s->net.tx.data, 1, "CCC", &uid, &stack_idx, &n);
      broadcast(s);
    }
    break;
  }
  case PCKT_POS: {
    unsigned char uid;
    ivec pos;
    unpack(s->net.rx.data, 8, "QQC", &pos.x, &pos.y, &uid);
    if (s->sdata.server) uid = true_uid(s);
    // here so if the player is not found, and -1 wraps around
    if (uid > SOLR_MAX_PLAYERS) break;
    s->st.players[uid].pos = pos;
    
    if (s->sdata.server) {
      s->net.tx.kind = p->kind;
      s->net.tx.data_len = pack(s->net.tx.data, 8, "QQC", &pos.x, &pos.y, &uid);
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
      ssize_t stack_idx = bbox_check(game, mouse_pos);
      //printf("bbox %zd @ %ld,%ld\n", stack_idx, mouse_pos.x, mouse_pos.y);
      unsigned char n = 1; // TODO/FIXME implement sub-bbox
      if (stack_idx < 0) break;
      game->net.tx = (netpacket){0};
      game->net.tx.kind = myp->held.stack.top > 0 ? PCKT_PUSH : PCKT_POP;
      game->net.tx.data_len = pack(game->net.tx.data, 1, "CCC", &game->client.uid, &stack_idx, &n);
      game->net.peer = game->sdata.server_a;
      netmgr_send(&game->net);
      break;
    }
    /*
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
card_render(solr_rdr *rdr, card *c, ivec pos) {
  SDL_Rect rect;
  SDL_Rect dst = posdims2rect(pos, (ivec){88, 124});
  SDL_Texture *atlas;

  if (!c->faceup) {
    atlas = rdr->backs;
    rect.x = 0; rect.w = 88;
    rect.h = 0; rect.h = 124;
  }
  else {
    atlas = rdr->suits[c->s];
    rect.x = ((c->r - 1) % 5) * 88;
    rect.y = ((c->r - 1) / 5) * 124;
    rect.w = 88; rect.h = 124;
  }
  SDL_RenderCopy(rdr->rdr, atlas, &rect, &dst);
}

void
card_stack_render(solr_rdr *rdr, card_stack *s, ivec pos) {
  SDL_Rect dst = posdims2rect(pos, (ivec){88, 124});
  if (s->top == 0) {
    SDL_Rect rect = {.x = 0, .y = 0, .w = 88, .h = 124};
    SDL_RenderCopy(rdr->rdr, rdr->slot, &rect, &dst);
    return;
  }
  if (s->pile) {
    SDL_Rect rect = {.x = 88*2, .y = 0, .w = 88, .h = 140};
    dst.h = rect.h;
    SDL_RenderCopy(rdr->rdr, rdr->piles, &rect, &dst);
  }

  if (!s->splayed) {
    card_render(rdr, &s->s[s->top - 1], pos);
  } else {
    for (size_t i = 0; i < s->top; i++) {
      card_render(rdr, &s->s[i], pos);
      pos.y += s->dims.y/s->top;
    }
  }
}

void
solr_render(scheduler *s, double a) {
  solr *game = (solr *)s->persistent_data;
  solr_rdr *r = &game->rdr;
  

  SDL_SetRenderDrawColor(r->rdr, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(r->rdr);

  for (size_t i = 0; i < STACK_MAX; i++) {
    card_stack_render(r, game->st.board.stacks + i, (ivec){stack_areas[i].x, stack_areas[i].y});
  }

  for (size_t i = 0; i < SOLR_MAX_PLAYERS; i++) {
    player *p = game->st.players + i;
    if (p->valid) card_stack_render(r, &p->held.stack, p->pos);
  }

  // triggers the double buffers for multiple rendering
  SDL_RenderPresent(r->rdr);
}

// FIXME: player held_card should be held_stack
// TODO: game state serde
// TODO: stack randomization
