#ifndef CROK_UNIT_HG__
#define CROK_UNIT_HG__

#include "crok/config.h"
#include "crok/phys.h"
#include "ivec.h"
#include <stdint.h>

enum crok_player_flags {
  CROK_PLAYER_VALID = 1 << 0,
  CROK_PLAYER_TEAM1 = 1 << 1,
  CROK_PLAYER_TEAM2 = 1 << 2,
};

struct crok_player {
  uint8_t id;
  struct ivec pos;
  char name[CROK_NAMELEN_MAX];
  enum crok_player_flags flags;
  uint32_t score;
};

enum crok_disc_flags { CROK_DISC_VALID = 1 << 0, CROK_DISC_FALLEN = 1 << 1 };

struct crok_disc {
  uint8_t id;
  uint8_t owner_id;
  struct physbody pb;
  enum crok_disc_flags flags;
};

#endif // CROK_UNIT_HG__
