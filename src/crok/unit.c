#include "crok/unit.h"

int crok_unit_team(const void *unit) {
  return *((enum crok_unit_flags *)unit) & (CROK_U_TEAM1 | CROK_U_TEAM2);
}

int crok_unit_valid(const void *unit) {
  return *((enum crok_unit_flags *)unit) & CROK_U_VALID;
}

ssize_t crok_unit_vacantpos(const void *unitarr, size_t elsize, size_t n) {
  ssize_t i = 0;
  char *cur = (char *)unitarr;
  for (i = 0; i < n && crok_unit_valid((void *)cur); i++) {
    cur += elsize;
  }
  if (i >= n) {
    i = -1;
  }
  return i;
}

void crok_player_print(struct crok_player *p) {
  printf("Player %s, flags = %d, pos = (%ld, %ld), score = %d\n", p->name,
         p->flags, p->pos.x, p->pos.y, p->score);
}
// TODO, FIXME, IMPLEMENT
void crok_disc_print(struct crok_player *d) { printf("Disc lol\n"); }
