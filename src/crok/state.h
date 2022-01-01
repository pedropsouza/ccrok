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

#define CROK_STATE_SERLEN                                                      \
  (sizeof(int8_t) * 2 + CROK_PLAYER_SERLEN * 4 +                               \
   CROK_DISC_SERLEN * CROK_DISCS_TOTAL + sizeof(uint32_t) * 2 +                \
   sizeof(uint16_t))
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
char *crok_state_serialize(const struct crok_state *st);
struct crok_state crok_state_deserialize(char buf[CROK_STATE_SERLEN]);

#endif // CROK_STATE_HG__
