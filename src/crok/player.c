#include "crok/player.h"

struct crok_player crok_player_init(uint8_t id, char name[CROK_NAMELEN_MAX]) {
  struct crok_player p = {};
  memcpy(p.name, name, CROK_NAMELEN_MAX - 1);
  p.name[CROK_NAMELEN_MAX - 1] = '\0';
  p.flags = 0;
  p.id = id;
  p.score = 0;
  return p;
}

int crok_player_team(struct crok_player *p) {
  return p->flags & (CROK_TEAM1 | CROK_TEAM2);
}

int crok_player_isvalid(struct crok_player *p) {
  return p->flags & CROK_PLAYER_VALID;
}

enum crok_gamestage crok_player_tick(struct crok_state *s) {
  // TODO
  return s->gamestage;
}
