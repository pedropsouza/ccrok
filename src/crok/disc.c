#include "crok/disc.h"

struct crok_disc crok_disc_init(struct crok_state *s, uint8_t owner) {
  struct crok_disc d = {.id = 0, // FIXME,
                        .owner_id = owner,
                        .pb = physbody_zero("disc"),
                        .flags = CROK_DISC_VALID};
  d.pb.pos.x = CROK_BOARD_DIAM / 2;
  d.pb.pos.y = CROK_BOARD_DIAM / 2;
  d.pb.radius = CROK_DISC_DIAM;
  return d;
}

int crok_disc_team(struct crok_disc *d, struct crok_state *s) {
  return crok_player_team(&s->players[d->owner_id]);
}

int crok_disc_isvalid(struct crok_disc *d) {
  return d->flags & CROK_DISC_VALID;
}

enum crok_gamestage crok_disc_tick(struct crok_disc *d, struct crok_state *s) {
  physbody_tick(&d->pb);
  return s->gamestage;
}

void crok_disc_render(struct crok_disc *d[2], struct crok_render_data *rd) {
  SDL_Rect dest = {0};
  double s = rd->rboard->disc_radius;
  struct fvec spos = crok_board_to_screen(rd, d[0]->pb.pos);
  dest.w = s * 2;
  dest.h = s * 2;
  dest.x = spos.x - s;
  dest.y = spos.y - s;

  SDL_RenderCopy(rd->rdr, rd->rboard->disc_tex, NULL, &dest);
}
