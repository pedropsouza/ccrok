#include "volksrace.h"
#include "serde.h"
#include "volks_serde.h"
#include "fvec.h"
#include "net.h"

static void
serde_init() {
  serde_fun *t = serializer_table;
  t['c'] = car_serde;
  t['f'] = fvec_serde;
  t['P'] = player_serde;
  t['p'] = physbody_serde;
  t['w'] = wheel_dyndata_serde;
}

void
broadcast(volks *s) {
  for (int i = 0; i < REAL_PLAYERS; i++) {
    if (!s->st.players[i].valid) continue;
    s->net.peer = s->srv.socks[i];
    netmgr_send(&s->net);
  }
}

sch_trans
volks_setup(scheduler *s) {
  setup_data *sdata = (setup_data *)s->setup_data;
  s->persistent_data =
      safe_calloc(sizeof (volks), "setup persistent_data alloc");
  volks *game = (volks *)s->persistent_data;

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
  game->net.spkinds.onerror = PCKT_INVALID;

  struct sockaddr_in *saddr = &game->sdata.server_a;
  size_t addrl = sizeof (*saddr);

  if (game->sdata.server) {
    saddr->sin_addr.s_addr = INADDR_ANY;

    int retc = bind(game->net.sock_fd, (struct sockaddr *)saddr, addrl);
    if (retc < 0) {
      perror("Couldn't bind network socket");
      goto neterr;
    }
  
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
    game->net.tx.data_len = pack(game->net.tx.data, 1, MACROSTR(volks_NAME_LEN) "s", game->sdata.username);
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
volks_cleanup(scheduler *s) {
  volks *game = (volks*)s->persistent_data;
  if (!game->sdata.server) {
    graphics_finish(game);
  }
  free(s->persistent_data);
  s->persistent_data = NULL;

  return (sch_trans){.s = SCHED_HALT, .why = "cleanup finished"};
}

ssize_t server_sockaddr2uid(server *s, struct sockaddr_in *sock) {
  ssize_t loc = -1;
  for (size_t i = 0; i < MAX_PLAYERS; i++) {
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
true_uid(volks *s) {
  return server_sockaddr2uid(&s->srv, &s->net.peer);
}

void
handle_pckt(volks *s, netpacket *p) {
  int8_t uid;
  switch (p->kind) {
    case PCKT_JOIN: {
      if (!s->sdata.server) return;
      unsigned char vacant = 0xff;
      for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!s->st.players[i].valid) {
          vacant = i;
          break;
        }
      }
      if (vacant == 0xff) {} // TODO

      s->st.players[vacant].car.body.pos = (fvec){0,0};
      s->st.players[vacant].valid = 1;
        
      s->srv.socks[vacant] = s->net.peer;
        
      if (p->data_len > 4 && p->data_len < NAME_LEN + 4) {
        unpack(p->data, 4, "s", &s->st.players[vacant].name);
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
        s->net.tx.kind = PCKT_STDUMP;
        s->net.tx.data_len = pack(s->net.tx.data, 8, "G", &s->st);
        broadcast(s);
        printf("got restart\n");
      }
      break;
    }
  }
}

float
lerp(float a, float b, float v) {
  return (a * (1.0 - v) + (b * v));
}

sch_trans
volks_fixed(scheduler *s, long step_ns) {
  volks *game = (volks *)s->persistent_data;
  player *my_player = game->st.players + game->client.uid;
  game->st.tick += 1;

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
        struct fvec pos = {.x = input.motion.x, .y = input.motion.y};
        pinput inp = {
          .force = fvec_sub(pos, game->st.players[game->client.uid].car.body.pos),
          .player_id = game->client.uid
        };
	      game->net.tx.kind = PCKT_PLAYER_INPUT;
      	game->net.tx.data_len = pinput_serde(game->net.tx.data, SERDE_MODE_PACK, &inp);
        netmgr_send(&game->net);
      }
      break;
    
    case SDL_MOUSEBUTTONUP: {
      player *myp = game->st.players + game->client.uid;
      if (input.button.button == SDL_BUTTON_RIGHT) {
        // game->net.tx.kind = PCKT_CANCEL;
        // game->net.tx.data_len = pack(game->net.tx.data, 1, "C", &game->client.uid);
        // netmgr_send(&game->net);
        break;
      }
      break;
    }
    
    case SDL_KEYUP:
      switch (input.key.keysym.sym) {
        case SDLK_RETURN:
        case SDLK_RETURN2:
        case SDLK_KP_ENTER:
          break;
        case SDLK_SPACE: {
          // game->net.tx.kind = PCKT_MOVE;
          // game->net.tx.data_len = pack(game->net.tx.data, 1, "m", &m);
          // netmgr_send(&game->net);
          break;
        }
        case SDLK_r: {
          game->net.tx.kind = PCKT_RESTARTREQ;
          game->net.tx.data_len = 0;
          netmgr_send(&game->net);
          break;
        }
        case SDLK_f: {
          // game->net.tx.kind = PCKT_FFWREQ;
          // game->net.tx.data_len = 0;
          // netmgr_send(&game->net);
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
player_render(volks_rdr *rdr, player *p) {
  SDL_SetRenderDrawColor(rdr->rdr, 0xff, 0x20, 0x20, 0xb0);
  SDL_RenderFillCircle(rdr->rdr, p->car.body.pos.x, p->car.body.pos.y, 3);
}

void
volks_render(scheduler *s, double a) {
  volks *game = (volks *)s->persistent_data;
  volks_rdr *r = &game->rdr;

  SDL_SetRenderDrawColor(r->rdr, 0x3c, 0x3c, 0x3c, SDL_ALPHA_OPAQUE);

  for (size_t i = 0; i < MAX_PLAYERS; i++) {
    player *p = game->st.players + i;
    if (p->valid) {
      player_render(r, p);
    }
  }

  // triggers the double buffers for multiple rendering
  end: SDL_RenderPresent(r->rdr);
}

