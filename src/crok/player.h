#ifndef CROK_PLAYER_HG__
#define CROK_PLAYER_HG__

#include "crok/data.h"

struct crok_player *crok_player_new(struct crok_state *s, int side,
                                    crok_input_producer_fun inputf, char *name);
int crok_player_team(struct crok_player *);
int crok_player_isvalid(struct crok_player *);
enum crok_gamestage crok_player_tick(struct crok_player *, struct crok_state *);
void crok_player_render(struct crok_player *p[2], SDL_Renderer *r, double a);

#endif // CROK_PLAYER_HG__
