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
  crok_spawn_player(&cs,
                    crok_player_init(CROK_U_TEAM1 | CROK_U_P1, "local player"));
  crok_spawn_player(
      &cs, crok_player_init(CROK_U_TEAM1 | CROK_U_P2, "remote player1"));
  crok_spawn_player(
      &cs, crok_player_init(CROK_U_TEAM2 | CROK_U_P1, "remote player2"));
  crok_spawn_player(
      &cs, crok_player_init(CROK_U_TEAM2 | CROK_U_P2, "remote player3"));
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

  pbuf_t buf[sizeof(struct crok_state)];
  size_t len = crok_state_pack(buf, &cs);

  int32_t first = crc_32((unsigned char *)&buf, len);
  cs.discs[4].pb.pos = (struct ivec){10, 60};

  len = crok_state_pack(buf, &cs);
  int32_t second = crc_32((unsigned char *)&buf, len);

  TEST_ASSERT(first != second);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(chksumTest);
  return UNITY_END();
}
