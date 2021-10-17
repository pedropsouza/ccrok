#include "crok/disc.h"

struct crok_disc crok_disc_init(struct crok_state *s, uint8_t owner) {
  struct crok_disc d = {.id = 0, // FIXME,
                        .owner_id = owner,
                        .pos = fvec_xy(0, 0),
                        .vel = fvec_xy(0, 0),
                        .acc = fvec_xy(0, 0),
                        .radius = CROK_DISC_DIAM,
                        .flags = CROK_DISC_VALID,
                        .tex = s->rboard.disc_tex};
  return d;
}

int crok_disc_team(struct crok_disc *d, struct crok_state *s) {
  return crok_player_team(&s->players[d->owner_id]);
}

int crok_disc_isvalid(struct crok_disc *d) {
  return d->flags & CROK_DISC_VALID;
}

enum crok_gamestage crok_disc_tick(struct crok_disc *d, struct crok_state *s) {
  // TODO
  return s->gamestage;
}

void crok_disc_render(struct crok_disc *d[2], struct crok_render_data *rd) {
  SDL_Rect dest = {0};
  double s = rd->rboard->disc_radius;
  dest.w = dest.h = s;
  dest.x = d[0]->pos.x - dest.w / 2;
  dest.y = d[0]->pos.y - dest.h / 2;

  SDL_RenderCopy(rd->r, d[0]->tex, NULL, &dest);
}
