#include "crok/crok.h"
#include "unity.h"
#include <stdio.h>

void setUp() {}
void tearDown() {}

void state_equal(a, b) struct crok_state *a, *b;
{
#define RIF(x) TEST_ASSERT(!(x))
#define IVECEQ(j, k) (j.x == k.x && j.y == k.y)

  RIF(a->active_player != b->active_player);
  RIF(a->gamestage != b->gamestage);
  RIF(a->overall_score != b->overall_score);
  RIF(a->round != b->round);
  for (int i = 0; i < 4; i++) {
    struct crok_player *l = &a->players[i], *r = &b->players[i];
    RIF(l->flags != r->flags);
    RIF(strcmp(l->name, r->name));
    RIF(!IVECEQ(l->pos, r->pos));
    RIF(l->score != r->score);
  }
  for (int i = 0; i < CROK_DISCS_TOTAL; i++) {
    struct crok_disc *l = &a->discs[i], *r = &b->discs[i];
    RIF(l->flags != r->flags);
    RIF(!IVECEQ(l->pb.pos, r->pb.pos));
    RIF(!IVECEQ(l->pb.vel, r->pb.vel));
    RIF(!IVECEQ(l->pb.acc, r->pb.acc));
    RIF(!IVECEQ(l->pb.facc, r->pb.facc));
    RIF(l->pb.radius != r->pb.radius);
    RIF(strcmp(l->pb.name, r->pb.name));
  }
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
  state_equal(&cs, &deser);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(state_serde);
  return UNITY_END();
}
