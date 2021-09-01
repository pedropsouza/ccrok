#ifndef CROK_HG__
#define CROK_HG__

#include <stdint.h>

struct crok_player {
  uint8_t id;
  uint8_t *name;
  uint8_t side;
  uint32_t score;
};

struct crok_state {
  // either 2 or 4, as 2 sides
  uint8_t n_players;
  uint8_t *players;
  uint8_t n_discs;
};

#endif // CROK_HG__
