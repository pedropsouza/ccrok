#ifndef CROK_GAMELOOP_HG__
#define CROK_GAMELOOP_HG__

#include "data.h"
#include "unit.h"
#include "crok/net.h"


typedef void (*crok_stage_gameloop_func)(struct crok_data *, netpacket *);
extern crok_stage_gameloop_func crok_stage_gameloops[CROK_GS_END];

struct crok_player crok_player_init(uint16_t flags, char *name);
void crok_player_render(struct crok_player *p[2], struct crok_render_data *rd);

struct crok_disc crok_disc_init(struct crok_player *owner);
void crok_disc_tick(struct crok_disc *, struct crok_state *);
void crok_disc_render(struct crok_disc *d[2], struct crok_render_data *rd);


// Returns either positive unit uid, or negative on failure
ssize_t crok_spawn_player(struct crok_state *cs, struct crok_player p);

// Returns either positive unit uid, or negative on failure
ssize_t crok_spawn_disc(struct crok_state *cs, struct crok_player *owner);

#endif // CROK_GAMELOOP_HG__
