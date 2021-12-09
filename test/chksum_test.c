#include "crok/crok.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void chksumTest() {
  struct crok_state cs = crok_state_init();
  crok_spawn_player(&cs, crok_player_init(CROK_U_TEAM1, 0, "local player"));
  crok_spawn_player(&cs, crok_player_init(CROK_U_TEAM1, 1, "remote player1"));
  crok_spawn_player(&cs, crok_player_init(CROK_U_TEAM2, 0, "remote player2"));
  crok_spawn_player(&cs, crok_player_init(CROK_U_TEAM2, 1, "remote player3"));
  crok_spawn_disc(&cs, &cs.players[0]);
  crok_spawn_disc(&cs, &cs.players[0]);
  crok_spawn_disc(&cs, &cs.players[0]);
  crok_spawn_disc(&cs, &cs.players[0]);
  crok_spawn_disc(&cs, &cs.players[0]);
  crok_spawn_disc(&cs, &cs.players[0]);
  crok_spawn_disc(&cs, &cs.players[2]);
  crok_spawn_disc(&cs, &cs.players[3]);
  crok_spawn_disc(&cs, &cs.players[3]);
  crok_spawn_disc(&cs, &cs.players[3]);
  crok_spawn_disc(&cs, &cs.players[3]);
  crok_spawn_disc(&cs, &cs.players[3]);

  int64_t first = crok_state_chksum(&cs);
  cs.board.discs[4].pb.pos.x = 10;
  cs.board.discs[4].pb.pos.y = 60;
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(chksumTest);
  return UNITY_END();
}
