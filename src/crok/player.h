#ifndef CROK_PLAYER_HG__
#define CROK_PLAYER_HG__

#include "arpa/inet.h"
#include "crok/data.h"
#include "draw.h"
#include "string.h"

struct crok_player crok_player_init(uint16_t flags, char *name);
void crok_player_render(struct crok_player *p[2], struct crok_render_data *rd);
char *crok_player_serialize(const struct crok_player *p);
struct crok_player crok_player_deserialize(char buf[CROK_PLAYER_SERLEN]);

#endif // CROK_PLAYER_HG__
