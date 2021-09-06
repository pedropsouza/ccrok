#ifndef CROK_PLAYER_HG__
#define CROK_PLAYER_HG__

#include "crok/data.h"

struct crok_player crok_player_init(uint8_t id, char name[CROK_NAMELEN_MAX]);
int crok_player_team(struct crok_player *);
int crok_player_isvalid(struct crok_player *);
enum crok_gamestage crok_player_tick(struct crok_state *);

#endif // CROK_PLAYER_HG__
