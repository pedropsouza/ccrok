#include "crok/crok.h"
#include "unity.h"
#include <stdio.h>

void setUp() {}
void tearDown() {}

void player_serde() {
  struct crok_player p, pdeser;
  memset(&p, 0, sizeof(struct crok_player));
  memset(&pdeser, 0, sizeof(struct crok_player));
  pbuf_t pser[sizeof(struct crok_player)];
  size_t pserlen;

  p = crok_player_init(CROK_U_TEAM1 | CROK_U_P1, "Napoleon");
  p.pos.x = 0xdeadbeeffeed5;
  p.pos.y = 0xb19da7ab0a7;
  p.score = 12;

  pserlen = crok_player_pack(pser, &p);
  crok_player_unpack(pser, &pdeser);
  crok_player_print(&pdeser);

  TEST_ASSERT(p.flags == pdeser.flags);
  TEST_ASSERT(p.pos.x == pdeser.pos.x && p.pos.y == pdeser.pos.y);
  TEST_ASSERT_EQUAL_STRING(p.name, pdeser.name);
  TEST_ASSERT(p.score == pdeser.score);

  p = crok_player_init(CROK_U_TEAM2 | CROK_U_P2, "AAAAAAAAAAAAAAAAAAAA");

  pserlen = crok_player_pack(pser, &p);
  crok_player_unpack(pser, &pdeser);
  crok_player_print(&pdeser);

  TEST_ASSERT(p.flags == pdeser.flags);
  TEST_ASSERT(p.pos.x == pdeser.pos.x && p.pos.y == pdeser.pos.y);
  TEST_ASSERT_EQUAL_STRING(p.name, pdeser.name);
  TEST_ASSERT(p.score == pdeser.score);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(player_serde);
  return UNITY_END();
}
