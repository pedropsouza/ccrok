#include "crok.h"

struct crok_player crok_player_init(uint8_t id, char name[CROK_MAXNAMELEN]) {
  struct crok_player p = {};
  memcpy(p.name, name, CROK_MAXNAMELEN - 1);
  p.name[CROK_MAXNAMELEN - 1] = '\0';
  p.flags = 0;
  p.id = id;
  p.score = 0;
  return p;
}
uint8_t crok_player_team(struct crok_player *p) {
  return p->flags & (CROK_TEAM1 | CROK_TEAM2);
}
uint8_t crok_player_isvalid(struct crok_player *p) {
  return p->flags & CROK_PLAYER_VALID;
}
enum crok_gamestage crok_player_tick(struct crok_state *s) {
  // TODO
  return s->gamestage;
}

struct crok_disc crok_disc_init(uint8_t id, uint8_t owner, struct fvec pos) {
  struct crok_disc d = {.id = id,
                        .owner_id = owner,
                        .pos = pos,
                        .vel = fvec_xy(0, 0),
                        .acc = fvec_xy(0, 0),
                        .radius = CROK_DISC_RADIUS,
                        .flags = 0};
  return d;
}
uint8_t crok_disc_team(struct crok_disc *d, struct crok_state *s) {
  return crok_player_team(&s->players[d->owner_id]);
}
uint8_t crok_disc_isvalid(struct crok_disc *d) {
  return d->flags & CROK_DISC_VALID;
}
enum crok_gamestage crok_disc_tick(struct crok_state *s) {
  // TODO
  return s->gamestage;
}

struct crok_state crok_state_init(void) {
  struct crok_state s = {0};
  s.round = 1;
  s.gamestage = CROK_GAMESTART;
  s.overall_score = 0;
  for (uint8_t x = 0; x < 8; x++) {
    s.poles[x] = fvec_xy(cos(atan(1) * x), sin(atan(1) * x));
  }
  return s;
}
enum crok_gamestage crock_gameloop(struct crok_state *s) {
  return s->gamestage;
}
