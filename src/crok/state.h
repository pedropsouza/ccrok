#ifndef CROK_STATE_HG__
#define CROK_STATE_HG__

#include "crok/board.h"
#include "crok/unit.h"
#include <stdint.h>

enum crok_gamestage {
  CROK_LOBBY = 0,
  CROK_SIDESEL,
  CROK_ROUNDSTART,
  CROK_AIM,
  CROK_SHOT,
  CROK_VALID,
  CROK_FOUL,
  CROK_TALLY,
  CROK_ROUNDEND,
  CROK_LEAVE,
  CROK_ERROR
};

struct crok_state {
  int8_t active_player, total_players;
  // either 2, or 4 as 2 sides
  struct crok_player players[4];
  struct crok_disc discs[CROK_DISCS_TOTAL];
  uint32_t round;
  uint32_t overall_score;
  enum crok_gamestage gamestage;
  struct crok_board board;
};

struct crok_state crok_state_init();

#endif // CROK_STATE_HG__
