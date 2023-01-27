#ifndef CROK_GAMELOOP_HG__
#define CROK_GAMELOOP_HG__

#include "data.h"
#include "unit.h"

struct crok_player crok_player_init(uint16_t flags, char *name);
void crok_player_render(struct crok_player *p[2], struct crok_render_data *rd);

struct crok_disc crok_disc_init(struct crok_player *owner);
void crok_disc_tick(struct crok_disc *, struct crok_state *);
void crok_disc_render(struct crok_disc *d[2], struct crok_render_data *rd);

void crok_update_nameplates(struct crok_data *d);

#endif // CROK_GAMELOOP_HG__
