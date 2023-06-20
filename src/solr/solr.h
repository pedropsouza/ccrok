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
#include "draw.h"
#include "serde.h"
#include "phys.h"

#define SOLR_PORT 51743
#define SOLR_NAME_LEN 64
#define SOLR_REAL_PLAYERS 7
#define SOLR_AUTO_PLAYER 7
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
typedef struct physbody physbody;

typedef struct {
  struct sockaddr_in server_a;
  size_t window_w, window_h;
  char username[SOLR_NAME_LEN];
  long frametime_ns;
  int rdr_backend;
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
  uint8_t s;
  uint8_t r;
  uint8_t faceup;
  uint8_t clr;
} card;
size_t card_serde(pbuf_t *buf, char mode, void *vc);

typedef struct {
  physbody pb;
  card c;
  uint8_t valid;
} physical_card;

typedef struct {
  card s[13*4];
  uint8_t top;
  uint8_t splayed;
  uint8_t pile;
  uint8_t id;
  uint8_t lock_info;
} card_stack;
size_t card_stack_serde(pbuf_t *buf, char mode, void *s);
void push_card_stack(card_stack *dst, card_stack *src);
void take_card_stack(card_stack *src, card_stack *dst, size_t n);
int is_locked_stack(card_stack *stack);
int who_locked_stack(card_stack *stack);
void lock_stack(card_stack *stack, uint8_t who);
void unlock_stack(card_stack *stack);

typedef struct {
  card_stack stack;
  uint8_t camefrom;
} held_stack;

enum {
  MOVE_PUSH = 0,
  MOVE_TAKE = 1,
};
typedef struct {
  uint8_t kind;
  struct {
    uint8_t src;
    uint8_t dst;
  } stacks;
  // only matters when taking
  uint8_t num_cards;
} move;
size_t move_serde(pbuf_t *buf, char mode, void *m);

enum {
  STACK_STOCK = 0,
  STACK_TABLEAU_BEGIN = 1,
  STACK_TABLEAU_END = 7,
  STACK_SUITS_BEGIN = 8,
  STACK_SUITS_END = 11,
  STACK_WASTE = 12,
  STACK_BOARD_END = 12,
  STACK_BOARD_MAX = 13,
  STACK_PLAYER_BEGIN = 13,
  STACK_MAX = STACK_PLAYER_BEGIN + SOLR_MAX_PLAYERS,
};
typedef struct {
  card_stack stacks[STACK_BOARD_MAX];
} board;
size_t board_serde(pbuf_t *buf, char mode, void *b);

typedef struct {
  uint8_t valid;
  ivec pos;
  char username[SOLR_NAME_LEN];
  held_stack held;
} player;
size_t player_serde(pbuf_t *buf, char mode, void *p);

enum {
  STAGE_INVALID = 0,
  STAGE_PLAYING = 1,
  STAGE_WON = 2,
  STAGE_LOST = 3,
  STAGE_FFW = 4,
};

typedef struct {
  board board;
  player players[SOLR_MAX_PLAYERS];
  long tick;
  uint8_t stage;
  physical_card flung_card; // client-side only
} gamestate;
size_t gamestate_serde(pbuf_t *buf, char mode, void *st);

enum {
  PCKT_INVALID = 0,
  PCKT_ACK = 1,
  PCKT_JOIN = 10,
  PCKT_LEAVE = 11,
  PCKT_POS = 2,
  PCKT_MOVE = 4,
  PCKT_CANCEL = 5,
  PCKT_STDUMP = 3,
  PCKT_RESTARTREQ = 6,
  PCKT_FFWREQ = 7,
  PCKT_FFWANIM = 8,
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
  SDL_Texture *locked_icon;
  SDL_Texture *suits[SUIT_MAX];

  SDL_Rect stack_areas[STACK_MAX];
  ivec win_dims;
  ivec card_dims;
} solr_rdr;

typedef struct {
  gamestate st;
  netmgr net;
  server srv;
  client client;
  setup_data sdata;
  solr_rdr rdr;
} solr;

int move_valid(solr *s, move *m, char **reason);
typedef struct {card_stack *src,*dst;} card_stack_pair;

sch_trans solr_setup  (scheduler *s);
sch_trans solr_cleanup(scheduler *s);
sch_trans solr_fixed  (scheduler *s, long step_ns);
void      solr_render (scheduler *s, double a);

// autosolve

typedef struct {
  uint64_t lerpval;
  int src,dst;
} ffw_anim;
size_t ffw_anim_serde(pbuf_t *buf, char mode, void *st);

int is_ffw_able(solr *s);
void ffw(solr *s);
void ffw_animate_server(solr *s);
void ffw_animate_client(solr *s, ffw_anim *an);
void handle_pckt(solr *s, netpacket *p);

// misc. utils

card_stack* stack_from_id(solr *s, uint8_t id);
void update_stack_dims(solr* s, card_stack *ss);
void shuffle_stack(card_stack *s);
void expose_top_card_stack(card_stack *s);
card_stack_pair next_ffw_stacks(solr *s);
int yoff_card_stack(solr_rdr *rdr, card_stack *s);

void graphics_init(solr *s);
void graphics_finish(solr *s);
void graphics_resize(solr *s, ivec new_window_dims);

extern const ivec ref_window_dims;
extern const ivec card_dims;
float lerp(float a, float b, float v);
void broadcast(solr *s);

#endif // SOLR_HG__
