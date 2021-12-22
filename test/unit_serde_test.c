#include "crok/crok.h"
#include "unity.h"
#include <stdio.h>

void setUp() {}
void tearDown() {}

void player_serde() {
  struct crok_player p, deser;
  char *ser;

  printf("CROK_PLAYER_SERLEN %ld\n", CROK_PLAYER_SERLEN);

  p = crok_player_init(CROK_U_TEAM1 | CROK_U_P1, "Napoleon");
  p.pos.x = 0xdeadbeeffeed5;
  p.pos.y = 0xb19da7ab0a7;
  p.score = 12;

  ser = crok_player_serialize(&p);
  deser = crok_player_deserialize(ser);
  TEST_ASSERT_EQUAL_CHAR_ARRAY((char *)&p, (char *)&deser, CROK_PLAYER_SERLEN);

  p = crok_player_init(CROK_U_TEAM2 | CROK_U_P2, "AAAAAAAAAAAAAAAAAAAA");

  ser = crok_player_serialize(&p);
  deser = crok_player_deserialize(ser);
  TEST_ASSERT(p.flags == deser.flags);
  TEST_ASSERT(p.pos.x == deser.pos.x && p.pos.y == deser.pos.y);
  TEST_ASSERT_EQUAL_STRING(p.name, deser.name);
  TEST_ASSERT(p.score == deser.score);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(player_serde);
  return UNITY_END();
}
