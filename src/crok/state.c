#include "crok/state.h"

struct crok_state crok_state_init() {
  struct crok_state s = {0};
  s.round = 1;
  s.gamestage = CROK_LOBBY;
  s.overall_score = 0;
  s.board = crok_board_calc();

  return s;
}
