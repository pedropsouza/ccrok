#include "crok/disc.h"

struct crok_disc crok_disc_init(struct crok_player *owner) {
  struct crok_disc d = {.pb = physbody_zero("disc"), .flags = owner->flags};
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

char *crok_disc_serialize(const struct crok_disc *d) {
  static char buf[CROK_DISC_SERLEN];

  char *w_head = buf;
  *((uint32_t *)w_head) = htons(d->flags);
  w_head += sizeof(uint32_t);
  memcpy(w_head, physbody_serialize(&d->pb), PHYSBODY_SERLEN);

  return buf;
}
struct crok_disc crok_disc_deserialize(char buf[CROK_DISC_SERLEN]) {
  struct crok_disc d;

  char *r_head = buf;
  d.flags = ntohs(*((uint16_t *)r_head));
  r_head += sizeof(uint32_t);
  d.pb = physbody_deserialize(r_head);

  return d;
}
