#include "crok/state.h"

struct crok_state
crok_state_init() {
  struct crok_state s = {0};
  s.round = 1;
  s.stage.kind = CROK_GS_LOBBY;
  s.overall_score = 0;

  return s;
}

void
crok_state_print(struct crok_state *cs) {
  printf("State on stage %d round %d, with %d players and %d score\n",
    cs->stage.kind, cs->round, cs->total_players, cs->overall_score);
  for (int i = 0; i < 4; i++) {
    if(!crok_unit_valid(cs->players + i)) continue;
    if (cs->stage.kind == CROK_GS_ROUND && i == cs->stage.data.active_player)
      printf("[active] ");
    printf("%d ", i);
    crok_player_print(cs->players + i);
  }
  for (int i = 0; i < CROK_DISCS_TOTAL; i++) {
    if(!crok_unit_valid(cs->discs + i)) continue;
    printf("%d ", i);
    crok_disc_print(cs->discs + i);
  }
}

const char* crok_gstage_kind_name[CROK_GS_END] = { 
  [CROK_GS_LOBBY] = "lobby",
  [CROK_GS_SIDESEL] = "sidesel",
  [CROK_GS_ROUND] = "gameround",
  [CROK_GS_LEAVE] = "leave",
  [CROK_GS_ERROR] = "error",
};
