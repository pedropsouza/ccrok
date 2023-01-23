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

bool player_equal(struct crok_player *a, struct crok_player *b)
{
  RIF(a->flags != b->flags);
  RIF(a->pos.x != b->pos.x && a->pos.y == b->pos.y);
  RIF_VEQNE(a->pos, b->pos);
  RIF(strcmp(a->name, b->name));
  RIF(a->score != b->score);
  return true;

}

bool disc_equal(struct crok_disc *a, struct crok_disc *b)
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
  alignas(8) pbuf_t pser[sizeof(struct crok_player)];
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

bool state_equal(struct crok_state *a, struct crok_state *b)
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
    RIF(!disc_equal(l,r));
  }
  return true;
}

void state_serde() {
  struct crok_state cs = {0}, deser = {0};
  cs = crok_state_init();
  size_t len;
  crok_spawn_player(&cs, crok_player_init(CROK_U_TEAM1 | CROK_U_P1, "Dan"));
  cs.active_player = crok_spawn_player(
      &cs, crok_player_init(CROK_U_TEAM2 | CROK_U_P1, "Malf"));

  cs.gamestage = CROK_ROUNDSTART;
  crok_spawn_disc(&cs, &cs.players[cs.active_player]);
  crok_spawn_disc(&cs, &cs.players[cs.active_player]);
  crok_spawn_disc(&cs, &cs.players[cs.active_player]);

  alignas(8) pbuf_t ser_a[sizeof(struct crok_state)];
  len = crok_state_pack(ser_a, &cs);
  TEST_ASSERT(len <= sizeof(struct crok_state));
  crok_state_unpack(ser_a, &deser);
  TEST_ASSERT(state_equal(&cs, &deser));
}

void ivec_serde() {
  struct ivec v,uv;

  alignas(8) pbuf_t ser[sizeof(struct ivec)];
  size_t len;

  for (int x = -100; x <= 100; x++) {
    for (int y = -100; y <= 100; y++) {
      len = ivec_pack(ser, &v);
      ivec_unpack(ser, &uv);
      TEST_ASSERT(v.x == uv.x && v.y == uv.y);
    }
  }
}

void netpacket_serde() {
  struct ivec v,uv;

  alignas(8) pbuf_t ser[sizeof(struct crok_net_packet)];
  size_t len;
  struct crok_net_packet pckt = {
    .ticknum = 0xdeadbeef,
    .statesum = 0xdead,
    .kind = CROK_NET_POS,
    .conn_uid = 0 //FIXME
	};

  struct crok_net_packet deser;

  for (int x = -10; x <= 10; x++) {
    for (int y = -10; y <= 10; y++) {
      pckt.data_len = ivec_pack(pckt.data, &v);
      len = netpacket_pack(ser, &pckt);
      netpacket_unpack(ser, &deser);
      TEST_ASSERT(pckt.data_len == deser.data_len);
      ivec_unpack(deser.data, &uv);
      TEST_ASSERT(v.x == uv.x && v.y == uv.y);
    }
  }

  printf("netpacked true size = %lu, naive size = %lu\n", len,
        calcsize("QICC") + 4 + calcsize("v"));
}

void alignment() {
  size_t ss = calcsize("24s");
  size_t vs = calcsize("v");
  size_t ps = calcsize("p");
  size_t Ps = calcsize("P");
  size_t Ds = calcsize("D");
  TEST_ASSERT_MESSAGE(ss == 26, "strings");
  TEST_ASSERT_MESSAGE(vs == 16, "ivecs");
  TEST_ASSERT_MESSAGE(ps == 4*calcsize("v") + 4 + 2 + PHYSBODY_NAME_MAXLEN, "physbodies");
  TEST_ASSERT_MESSAGE(ps % 4 == 0, "physbody alignment");
  TEST_ASSERT_MESSAGE(Ps == calcsize("v") + 4 + 2 + 2 + CROK_PLAYER_NAME_MAXLEN, "players");
  TEST_ASSERT_MESSAGE(Ps % 4 == 0, "player alignment");
  TEST_ASSERT_MESSAGE(Ds == calcsize("p") + 4, "discs");
  TEST_ASSERT_MESSAGE(Ds % 4 == 0, "disc alignment");
  size_t a = calcsize("S");
  size_t b = 4*calcsize("P") + CROK_DISCS_TOTAL*calcsize("D") + 4 + 4 + 2 + 1 + 1;
  printf("%lu == %lu", a, b);
  TEST_ASSERT_MESSAGE(a == b, "game state");
  TEST_ASSERT_MESSAGE(a % 4 == 0, "game state alignment");
  printf("ss = %lu, vs = %lu, ps = %lu, Ps = %lu, Ds = %lu, Ss = %lu\n", ss, vs, ps, Ps, Ds, a);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(player_serde);
  RUN_TEST(state_serde);
  RUN_TEST(ivec_serde);
  RUN_TEST(netpacket_serde);
  RUN_TEST(alignment);
  return UNITY_END();
}
