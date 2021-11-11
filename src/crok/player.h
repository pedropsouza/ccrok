#ifndef CROK_PLAYER_HG__
#define CROK_PLAYER_HG__

#include "crok/data.h"

// FIXME: should return the initialized player's index, or <0 for errors
struct crok_player *crok_append_player(struct crok_state *s, int side,
                                       char *name);
int crok_player_team(struct crok_player *);
int crok_player_isvalid(struct crok_player *);
void crok_player_render(struct crok_player *p[2], struct crok_render_data *rd);

#endif // CROK_PLAYER_HG__
