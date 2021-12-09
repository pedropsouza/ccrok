#include "crok/disc.h"

struct crok_disc crok_disc_init(struct crok_player *owner) {
  struct crok_disc d = {.owner_team_num = owner->team_num,
                        .pb = physbody_zero("disc"),
                        .flags = owner->flags};
  d.pb.pos.x = CROK_BOARD_DIAM / 2;
  d.pb.pos.y = CROK_BOARD_DIAM / 2;
  d.pb.radius = CROK_DISC_DIAM;
  return d;
}

enum crok_gamestage crok_disc_tick(struct crok_disc *d, struct crok_state *s) {
  physbody_tick(&d->pb);
  return s->gamestage;
}

void crok_disc_render(struct crok_disc *d[2], struct crok_render_data *rd) {
  SDL_Rect dest = {0};
  double s = rd->rboard->disc_diam;
  struct fvec spos = crok_board_to_screen(rd, d[0]->pb.pos);
  dest.w = s;
  dest.h = s;
  dest.x = spos.x - (s - 1.0) / 2.0;
  dest.y = spos.y - (s - 1.0) / 2.0;

  SDL_RenderCopy(rd->rdr, rd->rboard->disc_tex, NULL, &dest);
}
