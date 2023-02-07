#ifndef SOLR_HG__
#define SOLR_HG__

#include <unistd.h>
#include "../../libcrc/include/checksum.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "fvec.h"
#include "memutil.h"
#include "sched.h"
#include "serde.h"

#define SOLR_PORT 51743
#define SOLR_NAME_LEN 64
#define SOLR_MAX_PLAYERS 8
#define SOLR_PACKET_MAX_PAYLOAD_LEN 8192

typedef struct scheduler scheduler;
typedef struct sched_trans sch_trans;
typedef struct fvec fvec;
typedef struct ivec ivec;

typedef struct {
  struct sockaddr_in server_a;
  size_t window_w, window_h;
  char username[SOLR_NAME_LEN];
  long frametime_ns;
  bool server;
} setup_data;

typedef enum {
  SUIT_H = 1,
  SUIT_D = 2,
  SUIT_C = 3,
  SUIT_S = 4
} suit;

typedef enum {
  RANK_A = 1,
  RANK_2 = 2,
  RANK_3 = 3,
  RANK_4 = 4,
  RANK_5 = 5,
  RANK_6 = 6,
  RANK_7 = 7,
  RANK_8 = 8,
  RANK_9 = 9,
  RANK_10 = 10,
  RANK_J = 11,
  RANK_Q = 12,
  RANK_K = 13,
} rank;

typedef struct {
  suit s;
  rank r;
  uint8_t clr;
} card;

typedef struct {
  card c;
  int camefrom;
} held_card;

typedef struct {
  card s[13*4];
  size_t top;
  bool splayed;
} card_stack;
void push_card_stack(card_stack *s, card c);
card pop_card_stack(card_stack *s);

typedef struct {
  // idx 0 => borrow, 1-7 => field, 8-12 => suits
  card_stack stacks[13];
} board;

typedef struct {
  bool valid;
  ivec pos;
  char username[SOLR_NAME_LEN];
  card held_card;
} player;

typedef struct {
  board board;
  player players[SOLR_MAX_PLAYERS];
  long tick;
} gamestate;

typedef struct {
  enum {
    PKCT_INVALID = 0,
    PCKT_ACK = 1,
    PCKT_JOIN = 10,
    PCKT_LEAVE = 11,
    PCKT_POS = 2,
    PCKT_PUSH = 4,
    PCKT_POP = 5,
    PCKT_STDUMP = 3,
    PCKT_END = 16,
  } kind;
  pbuf_t data[SOLR_PACKET_MAX_PAYLOAD_LEN];
  size_t data_len;
} packet;

typedef struct {
  int sock_fd;
  packet tx;
  packet rx;
  struct sockaddr_in peer;
} netmgr;

size_t netmgr_send(netmgr *n);
size_t netmgr_recv(netmgr *n);

typedef struct {
  struct sockaddr_in socks[SOLR_MAX_PLAYERS];
} server;
ssize_t server_sockaddr2uid(server *s, struct sockaddr_in *sock);

typedef struct {
  size_t uid;
} client;

typedef struct {
  
} solr_rdr;

typedef struct {
  gamestate st;
  netmgr net;
  server srv;
  client client;
  setup_data sdata;
} solr;

sch_trans solr_setup  (scheduler *s);
sch_trans solr_cleanup(scheduler *s);
sch_trans solr_fixed  (scheduler *s, long step_ns);
void      solr_render (scheduler *s, double a);

#endif // SOLR_HG__
