#ifndef SOLR_HG__
#define SOLR_HG__

#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
//#include "../../libcrc/include/checksum.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "ivec.h"
#include "memutil.h"
#include "sched.h"
#include "serde.h"

#define SOLR_PORT 51743
#define SOLR_NAME_LEN 64
#define SOLR_MAX_PLAYERS 8
#define NETPACKET_MAX_PAYLOAD 8192
#include "net.h"

// For expanding macros inside string literals
#define MACROSTR2(X) #X
#define MACROSTR(X) MACROSTR2(X)

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
  SUIT_BEG = 1,
  SUIT_H = 1,
  SUIT_D = 2,
  SUIT_C = 3,
  SUIT_S = 4,
  SUIT_MAX = 5,
} suit;

typedef enum {
  RANK_BEG = 1,
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
  RANK_MAX = 14,
} rank;

typedef struct {
  suit s;
  rank r;
  bool faceup;
  uint8_t clr;
  ivec dims;
} card;

typedef struct {
  card s[13*4];
  size_t top;
  bool splayed;
  bool pile;
  ivec dims;
} card_stack;
void push_card_stack(card_stack *dst, card_stack *src);
void pop_card_stack(card_stack *src, card_stack *dst, size_t n);

typedef struct {
  card_stack stack;
  int camefrom;
} held_stack;

enum {
  STACK_STOCK = 0,
  STACK_TABLEAU_BEGIN = 1,
  STACK_TABLEAU_END = 7,
  STACK_SUITS_BEGIN = 8,
  STACK_SUITS_END = 12,
  STACK_WASTE = 13,
  STACK_MAX = 14,
};
typedef struct {
  card_stack stacks[STACK_MAX];
} board;

typedef struct {
  bool valid;
  ivec pos;
  char username[SOLR_NAME_LEN];
  held_stack held;
} player;

typedef struct {
  board board;
  player players[SOLR_MAX_PLAYERS];
  long tick;
} gamestate;

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
};

typedef struct {
  struct sockaddr_in socks[SOLR_MAX_PLAYERS];
} server;
ssize_t server_sockaddr2uid(server *s, struct sockaddr_in *sock);

typedef struct {
  size_t uid;
} client;

typedef struct {
  SDL_Window *win;
  SDL_Renderer *rdr;
  
  SDL_Texture *piles;
  SDL_Texture *backs;
  SDL_Texture *slot;
  SDL_Texture *suits[SUIT_MAX];
} solr_rdr;

typedef struct {
  gamestate st;
  netmgr net;
  server srv;
  client client;
  setup_data sdata;
  solr_rdr rdr;
} solr;

sch_trans solr_setup  (scheduler *s);
sch_trans solr_cleanup(scheduler *s);
sch_trans solr_fixed  (scheduler *s, long step_ns);
void      solr_render (scheduler *s, double a);

#endif // SOLR_HG__
