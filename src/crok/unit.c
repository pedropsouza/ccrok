#include "crok/unit.h"

int crok_unit_team(const void *unit) {
  return *((enum crok_unit_flags *)unit) & (CROK_U_TEAM1 | CROK_U_TEAM2);
}

int crok_unit_valid(const void *unit) {
  return *((enum crok_unit_flags *)unit) & CROK_U_VALID;
}

size_t crok_unit_vacantpos(const void *unitarr, size_t elsize, size_t n) {
  size_t i = 0;
  char *cur = (char *)unitarr;
  for (i = 0; i < n && crok_unit_valid((void *)cur); i++) {
    cur += elsize;
  }
  if (i >= n) {
    i = -1;
  }
  return i;
}

char *crok_player_serialize(const struct crok_player *p) {
  static char buf[CROK_PLAYER_SERLEN];
  char *w_head = buf;
  memcpy(w_head, ivec_serialize(&p->pos), IVEC_SERLEN);
  w_head += IVEC_SERLEN;
  *((uint32_t *)w_head) = htonl((uint32_t)p->score);
  w_head += sizeof(uint32_t);
  strncpy(w_head, p->name, CROK_NAMELEN_MAX);
  w_head += CROK_NAMELEN_MAX;
  // Assuming the whole enum fits into a short uint
  *((uint16_t *)w_head) = htons((uint16_t)p->flags);
  return buf;
}

struct crok_player crok_player_deserialize(char buf[CROK_PLAYER_SERLEN]) {
  struct crok_player p;

  char *r_head = buf;
  p.pos = ivec_deserialize(r_head);
  r_head += IVEC_SERLEN;
  p.score = ntohl(*((uint32_t *)r_head));
  r_head += sizeof(uint32_t);
  strncpy(p.name, r_head, CROK_NAMELEN_MAX);
  r_head += CROK_NAMELEN_MAX;
  p.flags = ntohs(*((uint16_t *)r_head));
  return p;
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
