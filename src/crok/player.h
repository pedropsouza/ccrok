#ifndef CROK_PLAYER_HG__
#define CROK_PLAYER_HG__

#include "crok/data.h"

struct crok_player crok_player_init(int side, int team_num, char *name);
void crok_player_render(struct crok_player *p[2], struct crok_render_data *rd);

#endif // CROK_PLAYER_HG__
