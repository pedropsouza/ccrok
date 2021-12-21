#ifndef CROK_UNIT_HG__
#define CROK_UNIT_HG__

#include "crok/config.h"
#include "crok/phys.h"
#include "ivec.h"
#include <stdint.h>

enum crok_unit_flags {
  CROK_U_VALID = 1 << 0,
  CROK_U_TEAM1 = 1 << 1,
  CROK_U_TEAM2 = 1 << 2,
  CROK_U_P1 = 1 << 3,
  CROK_U_P2 = 1 << 4,
  CROK_U_FALLEN = 1 << 5,
};

#define CROK_PLAYER_SERLEN                                                     \
  (sizeof(uint16_t) + sizeof(uint32_t) * 5 + CROK_NAMELEN_MAX)
struct crok_player {
  enum crok_unit_flags flags;
  struct ivec pos;
  char name[CROK_NAMELEN_MAX];
  uint32_t score;
};

#define CROK_DISC_SERLEN (sizeof(uint16_t) + sizeof(physbody))
struct crok_disc {
  enum crok_unit_flags flags;
  struct physbody pb;
};

/* ! units should start with the flags enum else this breaks ! */
int crok_unit_team(const void *unit);
int crok_unit_valid(const void *unit);
size_t crok_unit_vacantpos(const void *unitarr, size_t elsize, size_t n);

#endif // CROK_UNIT_HG__
