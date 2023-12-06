#ifndef VOLKSRACE_HG__
#define VOLKSRACE_HG__

#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
//#include "../../libcrc/include/checksum.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "nuklear.h"
#include "ivec.h"
#include "fvec.h"
#include "memutil.h"
#include "sched.h"
#include "draw.h"
#include "serde.h"
#include "phys.h"

#define PORT 51743
#define NAME_LEN 64
#define REAL_PLAYERS 7
#define AUTO_PLAYER 7
#define MAX_PLAYERS 8
#define NETPACKET_MAX_PAYLOAD 8192
#include "net.h"

// For expanding macros inside string literals
#define MACROSTR2(X) #X
#define MACROSTR(X) MACROSTR2(X)

#define WHEEL_DYNDATA_FIELDS(F,AF) \
  F(fvec, pos_rel_car, "f") \
  F(float, weight, "I") \
  F(fvec, friction, "f") \
  F(float, contact_patch_area, "I") \
  F(float, static_friction_coeff, "I") \
  F(float, dyn_friction_coeff, "I") \
  F(uint8_t, state, "C") \

#define CAR_FIELDS(F,AF) \
  F(physbody, body, "p") \
  F(float, heading_angle, "I") \
  F(float, angular_vel, "I") \
  AF(wheel_dyndata, wheels, 4, "w") \
  F(float, steering_angle, "I") \
  F(float, throttle, "I") \
  F(fvec, wheel_force_sum, "f") \

#define PLAYER_FIELDS(F,AF) \
  F(car, car, "c") \
  AF(char, name, NAME_LEN, MACROSTR(NAME_LEN) "s") \
  F(int, valid, "C") \

#define GAMESTATE_FIELDS(F,AF) \
  AF(player, players, MAX_PLAYERS, MACROSTR(MAX_PLAYERS) "P") \
  F(long, tick, "Q") \
  F(uint8_t, stage, "C") \

#define PINPUT_FIELDS(F,AF) \
  F(fvec, force, "f") \
  F(uint8_t, player_id, "C") \

#define SYNCED_STRUCTS(ST) \
  ST(wheel_dyndata, WHEEL_DYNDATA_FIELDS) \
  ST(car, CAR_FIELDS) \
  ST(player, PLAYER_FIELDS) \
  ST(gamestate, GAMESTATE_FIELDS) \
  ST(pinput, PINPUT_FIELDS) \

typedef struct scheduler scheduler;
typedef struct sched_trans sch_trans;
typedef struct fvec fvec;
typedef struct ivec ivec;
typedef struct physbody physbody;
typedef struct volks volks;

#define TYPEDEF_DECL(NAME, DEFBLOCK) typedef struct NAME NAME;
SYNCED_STRUCTS(TYPEDEF_DECL)

typedef struct {
  struct sockaddr_in server_a;
  size_t window_w, window_h;
  char username[NAME_LEN];
  long frametime_ns;
  int rdr_backend;
  bool server;
} setup_data;

enum {
  STAGE_INVALID = 0,
  STAGE_PLAYING = 1,
};

/* ------- CAR STUFF ------- */

enum wheel_state {
  WHEEL_NOMINAL,
  WHEEL_SLIPPING,
  WHEEL_SLIDING,
};

wheel_dyndata
*wheel_update(wheel_dyndata *w, fvec vel, float weight, float car_angle, float steer_angle);

#define DECL_FIELD(TYPE, NAME, FMTSTR) TYPE NAME;
#define DECL_ARRAY(TYPE, NAME, NELEM, FMTSTR) TYPE NAME[NELEM];
#define STRUCT_DECL(NAME, FIELDS) struct NAME { FIELDS(DECL_FIELD, DECL_ARRAY) };
SYNCED_STRUCTS(STRUCT_DECL)

void car_update(car *c);
void draw_car(volks *v, car *c);

typedef struct {
  struct sockaddr_in socks[MAX_PLAYERS];
} server;
ssize_t server_sockaddr2uid(server *s, struct sockaddr_in *sock);

typedef struct {
  size_t uid;
} client;

typedef struct {
  SDL_Window *win;
  SDL_Renderer *rdr;
  struct nk_context nkctx;
  struct nk_colorf bgclr;
  
  ivec win_dims;
} volks_rdr;

struct volks {
  gamestate st;
  netmgr net;
  server srv;
  client client;
  setup_data sdata;
  volks_rdr rdr;
};

sch_trans volks_setup  (scheduler *s);
sch_trans volks_cleanup(scheduler *s);
sch_trans volks_fixed  (scheduler *s, long step_ns);
void      volks_render (scheduler *s, double a);

// networking

enum {
  PCKT_INVALID = 0,
  PCKT_ACK = 1,
  PCKT_JOIN = 10,
  PCKT_LEAVE = 11,
  PCKT_PLAYER_INPUT = 2,
  PCKT_STDUMP = 3,
  PCKT_RESTARTREQ = 6,
  PCKT_END = 16,
};

void handle_pckt(volks *s, netpacket *p);

// gui

void gui_init(volks *s);
void gui_finish(volks *s);
void gui_render(volks *s);

void graphics_init(volks *s);
void graphics_finish(volks *s);
void graphics_resize(volks *s, ivec new_window_dims);

void draw_fvec(volks *v, fvec origin, fvec vec);

extern const ivec ref_window_dims;
float lerp(float a, float b, float v);
void broadcast(volks *s);

#endif // VOLKSRACE_HG__
