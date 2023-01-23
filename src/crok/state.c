#include "crok/state.h"

struct crok_state
crok_state_init() {
  struct crok_state s = {0};
  s.round = 1;
  s.gamestage = CROK_LOBBY;
  s.overall_score = 0;

  return s;
}

void
crok_state_print(struct crok_state *cs) {
  printf("State on stage %d round %d, with %d players and %d score\n",
    cs->gamestage, cs->round, cs->total_players, cs->overall_score);
  for (int i = 0; i < 4; i++) {
    if (i == cs->active_player) printf("[active] ");
    crok_player_print(cs->players + i);
  }
  for (int i = 0; i < CROK_DISCS_TOTAL; i++) {
    crok_disc_print(cs->discs + i);
  }
}
