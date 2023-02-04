#ifndef CROK_STATE_HG__
#define CROK_STATE_HG__

#include "crok/board.h"
#include "crok/unit.h"
#include <stdint.h>


enum crok_gstage_kind {
  CROK_GS_LOBBY = 0,
  CROK_GS_SIDESEL,
  CROK_GS_ROUND,
  CROK_GS_LEAVE,
  CROK_GS_ERROR,
  CROK_GS_END,
};
extern const char* crok_gstage_kind_name[CROK_GS_END];

struct crok_gstage {
  enum crok_gstage_kind kind;
  union {
    // lobby & sidesel
    uint8_t players_ready;
    
    // round
    uint8_t active_player;

    // error (local only, not networked)
    const char *why;
  } data;
};

struct crok_state {
  int8_t total_players;
  // either 2, or 4 as 2 sides
  struct crok_player players[4];
  struct crok_disc discs[CROK_DISCS_TOTAL];
  uint32_t round;
  uint32_t overall_score;
  struct crok_gstage stage;
};

struct crok_state crok_state_init();
void crok_state_print(struct crok_state *cs);

#endif // CROK_STATE_HG__
