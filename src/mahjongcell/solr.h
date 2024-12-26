#ifndef SOLR_HG__
#define SOLR_HG__

#include <unistd.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <stdbool.h>
//#include "../../libcrc/include/checksum.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "ivec.h"
#include "sched.h"
#include "draw.h"
#include "serde.h"
#include "phys.h"
#include "crc32.h"
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"

#define SOLR_PORT 51743
#define SOLR_NAME_LEN 64
#define SOLR_REAL_PLAYERS 7
#define SOLR_AUTO_PLAYER 7
#define SOLR_MAX_PLAYERS 8
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
  int username_len;
  long frametime_ns;
  int rdr_backend;
  bool server;
} setup_data;

typedef enum {
  SUIT_BEG = 1,
  SUIT_PIN = 1, // Blue
  SUIT_SOU = 2, // Green
  SUIT_MAN = 3, // Black
  SUIT_NORMAL_END = 3,
  SUIT_FLOWER = 4,
  SUIT_MAX = 5,
} suit;

typedef enum {
  RANK_BEG = 1,
  RANK_1 = 1,
  RANK_2 = 2,
  RANK_3 = 3,
  RANK_4 = 4,
  RANK_5 = 5,
  RANK_6 = 6,
  RANK_7 = 7,
  RANK_8 = 8,
  RANK_9 = 9,
  RANK_DRAGON = 10,
  RANK_MAX = 11,
} rank;

typedef struct {
  uint8_t s;
  uint8_t r;
  uint8_t faceup;
} card;
size_t card_serde(pbuf_t *buf, char mode, void *vc);

typedef struct {
  physbody pb;
  card c;
  uint8_t valid;
} physical_card;

typedef struct {
  card s[10*5];
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

typedef struct {card_stack *src,*dst;} card_stack_pair;

typedef struct {
  card_stack stack;
  uint8_t camefrom;
} held_stack;

enum {
  MOVE_PUSH = 0,
  MOVE_TAKE = 1,
  MOVE_GROUP_DRAGONS = 2,
};
typedef struct {
  uint8_t kind;
  union {
    struct {
      uint8_t src;
      uint8_t dst;
    } stacks;
    uint8_t suit;
  };
  // only matters when taking
  uint8_t num_cards;
} move;
size_t move_serde(pbuf_t *buf, char mode, void *m);

#define STACKS_XLIST \
  X(HOLDING_1, 0) \
  X(HOLDING_2, 1) \
  X(HOLDING_3, 2) \
  X(FLOWER, 3) \
  X(SUITS_BEGIN, 4) \
  X(SUITS_END, 6) \
  X(BOARD_BEGIN, 7) \
  X(BOARD_END, 14) \
  X(BOARD_MAX, 15) \
  X(PLAYER_BEGIN, 15) \
  X(MAX, STACK_PLAYER_BEGIN + SOLR_MAX_PLAYERS)

#define X(name, id) STACK_ ## name = id,
enum {
  STACKS_XLIST
};
#undef X

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
  STAGE_NETERR = 5,
};

typedef struct {
  board board;
  player players[SOLR_MAX_PLAYERS];
  uint32_t tick; // remember to keep type in sync with serde
  uint8_t stage;
  physical_card flung_card; // client-side only
} gamestate;
size_t gamestate_serde(pbuf_t *buf, char mode, void *st);

#define PACKET_KINDS \
  X(INVALID, 0) \
  X(ACK, 1) \
  X(POS, 2) \
  X(STDUMP, 3) \
  X(MOVE, 4) \
  X(CANCEL, 5) \
  X(RESTARTREQ, 6) \
  X(FFWREQ, 7) \
  X(FFWANIM, 8) \
  X(JOIN, 9) \
  X(LEAVE, 10) \
  X(DISCONNECT, 11) \
  X(END, 16) \
  X(MAX, PCKT_END+1)

#define X(name, num_expr) PCKT_ ## name = num_expr,
enum {
  PACKET_KINDS
};
#undef X
const char *packet_kind_to_name(int kind);

typedef struct {
  int listen_sock_fd;
  int sock_fds[SOLR_MAX_PLAYERS];
  struct sockaddr_storage sock_addresses[SOLR_MAX_PLAYERS];
  struct pollfd listen_sock_pfd;

  // costly call optimizations
  card_stack_pair ffwd_able_stacks;
} server;
ssize_t server_sock_fd2uid(server *s, int fd);

typedef struct {
  size_t uid;
} client;

typedef struct {
  SDL_Window *win;
  SDL_Renderer *rdr;
  
  SDL_Texture *backs;
  SDL_Texture *locked_icon;
  SDL_Texture *suits;

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
  struct nk_context *nk;
  int should_reload;
} solr;

int move_valid(solr *s, move *m, char **reason);
char *move_repr(solr *s, move *m);

#ifndef ENABLE_HOTRELOAD
solr solr_setup  (setup_data *s);
void solr_post_reload(solr *);
void solr_pre_reload (solr *);
void solr_cleanup(solr *);
int  solr_fixed  (solr *, long step_ns);
void solr_render (solr *, double a);
#endif

// networking/gameplay

typedef void (*pckt_handler)(solr *s, netpacket *p);
extern pckt_handler *pckt_handler_vec_base;
ssize_t true_uid(solr *s);
void setup_board_stacks(solr *game);
void almost_solved_stacks(solr *game);
int is_solved(solr *s);
void handle_pckt(solr *s, netpacket *p);

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

// gui

void gui_init(solr *s);
void gui_finish(solr *s);
void gui_update(solr *s);

// misc. utils

card_stack* stack_from_id(solr *s, uint8_t id);
void update_stack_dims(solr* s, card_stack *ss);
void shuffle_stack(card_stack *s);
void expose_top_card_stack(card_stack *s);
card_stack_pair next_ffw_stacks(solr *s);
int yoff_card_stack(solr_rdr *rdr, card_stack *s);
int stack_sequence_depth(card_stack *s);
char *stack_name_from_id(int id);

void graphics_init(solr *s);
void graphics_finish(solr *s);
void graphics_resize(solr *s, ivec new_window_dims);

extern const ivec ref_window_dims;
extern const ivec card_dims;
float lerp(float a, float b, float v);
void broadcast(solr *s);

// Hot reloading bits -- all tsoding's musializer code
// https://github.com/tsoding/musializer/blob/master/src/hotreload.h
#ifdef ENABLE_HOTRELOAD

#define HOT_FUNCS \
HOT_FUNC(solr_setup, solr, setup_data *s) \
HOT_FUNC(solr_post_reload, void, solr *) \
HOT_FUNC(solr_pre_reload,  void, solr *) \
HOT_FUNC(solr_cleanup, void, solr *) \
HOT_FUNC(solr_fixed, int, solr *, long) \
HOT_FUNC(solr_render, void, solr *, double)

#endif // ENABLE_HOTRELOAD

#endif // SOLR_HG__
