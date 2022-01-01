#include "crok/state.h"

struct crok_state crok_state_init() {
  struct crok_state s = {0};
  s.round = 1;
  s.gamestage = CROK_LOBBY;
  s.overall_score = 0;
  s.board = crok_board_calc();

  return s;
}

char *crok_state_serialize(const struct crok_state *cs) {
  static char buf[CROK_STATE_SERLEN];

  char *w_head = buf;
  for (int i = 0; i < 4; i++) {
    memcpy(w_head, crok_player_serialize(&(cs->players[i])),
           CROK_PLAYER_SERLEN);
    w_head += CROK_PLAYER_SERLEN;
  }
  for (int i = 0; i < CROK_DISCS_TOTAL; i++) {
    memcpy(w_head, crok_disc_serialize(&(cs->discs[i])), CROK_DISC_SERLEN);
    w_head += CROK_DISC_SERLEN;
  }
  *((uint32_t *)w_head) = htonl(cs->round);
  w_head += sizeof(uint32_t);
  *((uint32_t *)w_head) = htonl(cs->overall_score);
  w_head += sizeof(uint32_t);
  *((uint16_t *)w_head) = htons(cs->gamestage);
  w_head += sizeof(int16_t);
  *((int8_t *)w_head) = cs->active_player;
  w_head += sizeof(int8_t);
  *((int8_t *)w_head) = cs->total_players;

  return buf;
}

struct crok_state crok_state_deserialize(char buf[CROK_STATE_SERLEN]) {
  struct crok_state cs;

  char *r_head = buf;
  for (int i = 0; i < 4; i++) {
    cs.players[i] = crok_player_deserialize(r_head);
    r_head += CROK_PLAYER_SERLEN;
  }
  for (int i = 0; i < CROK_DISCS_TOTAL; i++) {
    cs.discs[i] = crok_disc_deserialize(r_head);
    r_head += CROK_DISC_SERLEN;
  }
  cs.round = ntohl(*((uint32_t *)r_head));
  r_head += sizeof(uint32_t);
  cs.overall_score = ntohl(*((uint32_t *)r_head));
  r_head += sizeof(uint32_t);
  cs.gamestage = ntohs(*((uint16_t *)r_head));
  r_head += sizeof(uint16_t);
  cs.active_player = *((uint8_t *)r_head);
  r_head += sizeof(uint8_t);
  cs.total_players = *((uint8_t *)r_head);

  cs.board = crok_board_calc();
  return cs;
}
