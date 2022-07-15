#include "crok/crok.h"
#include "unity.h"
#include <stdio.h>
#define RIF(x) if (x) { fprintf(stderr, "assertion at line " MACROSTR(__LINE__) " failed!\n"); return false; }
#define RIF_VEQNE(a, b) if (a.x != b.x || a.y != b.y) {\
  fprintf(stderr, "vector equality assertion at line " MACROSTR(__LINE__) " failed!\n");\
  ivec_print(a, "vector a");\
  ivec_print(b, "vector b");\
  return false; }

void setUp() {}
void tearDown() {}

bool player_equal(a, b) struct crok_player *a, *b;
{
  RIF(a->flags != b->flags);
  RIF(a->pos.x != b->pos.x && a->pos.y == b->pos.y);
  RIF_VEQNE(a->pos, b->pos);
  RIF(strcmp(a->name, b->name));
  RIF(a->score != b->score);
  return true;

}

bool disc_equal(a,b) struct crok_disc *a, *b;
{
  RIF(a->flags != b->flags);
  RIF_VEQNE(a->pb.pos, b->pb.pos);
  RIF_VEQNE(a->pb.vel, b->pb.vel);
  RIF_VEQNE(a->pb.acc, b->pb.acc);
  RIF_VEQNE(a->pb.facc, b->pb.facc);
  RIF(a->pb.radius != b->pb.radius);
  RIF(strcmp(a->pb.name, b->pb.name));
  return true;
}

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
  TEST_ASSERT(player_equal(&p, &pdeser));

  p = crok_player_init(CROK_U_TEAM2 | CROK_U_P2, "AAAAAAAAAAAAAAAAAAAA");

  pserlen = crok_player_pack(pser, &p);
  crok_player_unpack(pser, &pdeser);
  crok_player_print(&pdeser);
  TEST_ASSERT(player_equal(&p, &pdeser));
}

bool state_equal(a, b) struct crok_state *a, *b;
{
  RIF(a->active_player != b->active_player);
  RIF(a->gamestage != b->gamestage);
  RIF(a->overall_score != b->overall_score);
  RIF(a->round != b->round);
  for (int i = 0; i < 4; i++) {
    struct crok_player *l = &a->players[i], *r = &b->players[i];
    RIF(!player_equal(l,r));
  }
  for (int i = 0; i < CROK_DISCS_TOTAL; i++) {
    struct crok_disc *l = &a->discs[i], *r = &b->discs[i];
    RIF(!disc_equal(a,b));
  }
  return true;
}

void state_serde() {
  struct crok_state cs, deser;
  cs = crok_state_init();
  size_t len;
  crok_spawn_player(&cs, crok_player_init(CROK_U_TEAM1 | CROK_U_P1, "Dan"));
  cs.active_player = crok_spawn_player(
      &cs, crok_player_init(CROK_U_TEAM2 | CROK_U_P1, "Malf"));

  cs.gamestage = CROK_ROUNDSTART;
  crok_spawn_disc(&cs, &cs.players[cs.active_player]);
  crok_spawn_disc(&cs, &cs.players[cs.active_player]);
  crok_spawn_disc(&cs, &cs.players[cs.active_player]);

  pbuf_t ser[sizeof(struct crok_state) * 2];
  len = crok_state_pack(ser, &cs);
  crok_state_unpack(ser, &deser);
  TEST_ASSERT(state_equal(&cs, &deser));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(player_serde);
  RUN_TEST(state_serde);
  return UNITY_END();
}
