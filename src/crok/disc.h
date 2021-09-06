#ifndef CROK_DISC_HG__
#define CROK_DISC_HG__

#include "crok/data.h"
#include "crok/player.h"

struct crok_disc crok_disc_init(struct crok_state *s, uint8_t owner);
int crok_disc_team(struct crok_disc *, struct crok_state *);
int crok_disc_isvalid(struct crok_disc *);
enum crok_gamestage crok_disc_tick(struct crok_disc *, struct crok_state *);
void crok_disc_render(struct crok_disc *);

#endif // CROK_DISC_HG__
