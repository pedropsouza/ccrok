#ifndef CROK_DISC_HG__
#define CROK_DISC_HG__

#include "crok/data.h"
#include "crok/player.h"

struct crok_disc crok_disc_init(struct crok_player *owner);
enum crok_gamestage crok_disc_tick(struct crok_disc *, struct crok_state *);
void crok_disc_render(struct crok_disc *d[2], struct crok_render_data *rd);

char *crok_disc_serialize(const struct crok_disc *d);
struct crok_disc crok_disc_deserialize(char buf[CROK_DISC_SERLEN]);

#endif // CROK_DISC_HG__
