#include "solr/solr.h"
#include "unity.h"
#include <stdio.h>

void setUp() {}
void tearDown() {}

alignas(8) pbuf_t buf[NETPACKET_MAX_PAYLOAD];
size_t lenp, lenu;
void card_serde_test() {
  card c = {0}, cd = {0};

  for (int suit = SUIT_BEG; suit < SUIT_MAX; suit++) {
    int clr = suit <= 2;
    for (int rank = RANK_BEG; rank < RANK_MAX; rank++) {
      c = (card){.s = suit, .r = rank, .faceup = 0, .clr = clr};
      lenp = pack  (buf, 1, "c", &c);
      lenu = unpack(buf, 1, "c", &cd);
      TEST_ASSERT_EQUAL_MEMORY_ARRAY(&c, &cd, sizeof(card), 1);
    }
  }
}

void card_stack_init(card_stack *s) {
  for (int suit = SUIT_BEG; suit < SUIT_MAX; suit++) {
    int clr = suit <= 2;
    for (int rank = RANK_BEG; rank < RANK_MAX; rank++) {
      s->s[s->top] = (card){.s = suit, .r = rank, .faceup = 0, .clr = clr};
      s->top++;
    }
  }
}

void card_stack_serde_test() {
  card_stack s = {0}, sd = {0};

  card_stack_init(&s);
  lenp = pack  (buf, 8, "f", &s);
  lenu = unpack(buf, 8, "f", &sd);
  TEST_ASSERT_EQUAL_MEMORY_ARRAY(&s, &sd, sizeof(card_stack), 1);
}

void board_serde_test() {
  board b = {0}, bd = {0};

  for (int i = 0; i < STACK_BOARD_END; i++) {
    card_stack_init(b.stacks + i);
  }

  lenp = pack  (buf, 8, "b", &b);
  lenp = unpack(buf, 8, "b", &bd);
  TEST_ASSERT_EQUAL_MEMORY_ARRAY(&b, &bd, sizeof(board), 1);
}
/*
void state_serde() {
  struct crok_state cs = {0}, deser = {0};
  cs = crok_state_init();
  cs.stage.kind = CROK_GS_ROUND;
  size_t len;
  ssize_t uid1 = crok_spawn_player(&cs, crok_player_init(CROK_U_TEAM1 | CROK_U_P1, "Dan"));
  ssize_t uid2 = cs.stage.data.active_player = crok_spawn_player(
      &cs, crok_player_init(CROK_U_TEAM2 | CROK_U_P1, "Malf"));

  crok_spawn_disc(&cs, &cs.players[uid1]);
  crok_spawn_disc(&cs, &cs.players[uid1]);
  crok_spawn_disc(&cs, &cs.players[uid2]);

  alignas(8) pbuf_t ser_a[sizeof(struct crok_state)];
  len = crok_state_pack(ser_a, &cs);
  TEST_ASSERT(len <= sizeof(struct crok_state));
  crok_state_unpack(ser_a, &deser);
  TEST_ASSERT_EQUAL_MEMORY_ARRAY(&cs, &deser, sizeof(struct crok_state), 1);
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

enum t_kind { KIND_CHAR, KIND_I16, KIND_U32, KIND_FPDP, KIND_END };
char *t_fmtstrs[KIND_END] = {
  [KIND_CHAR] = "C", [KIND_I16] = "H", [KIND_U32] = "I", [KIND_FPDP] = "Q"
};
struct t {
  enum t_kind kind;
  union {
    char c;
    uint16_t i16;
    uint32_t u32;
    double fpdp;
  } data;
};

size_t tagged_struct_formatter(pbuf_t *head, char mode, void* io) {
  size_t s = 0;
  size_t (*f)(pbuf_t *, size_t, char *, ...) = (mode == SERDE_MODE_PACK)? pack : unpack;
  struct t *iot = (struct t*)io;
  s += (*f)(head, alignof(struct t), t_fmtstrs[iot->kind], &iot->data);
  return s;
}

void tagged_struct_serde() {
  struct t a, b;
  size_t size[2];

  alignas(alignof(struct t))
  pbuf_t serbuf[sizeof(struct t)*2];

  a = (struct t){0}; b = (struct t){0};
  a = (struct t){.kind = KIND_CHAR, .data.c = -3 }; 
  size[0] = pack(serbuf, alignof(struct t), "[C]", &tagged_struct_formatter, &a);
  size[1] = unpack(serbuf, alignof(struct t), "[C]", &tagged_struct_formatter, &b);
  TEST_ASSERT_EQUAL_MESSAGE(size[0], size[1], "tagged struct size"); 
  TEST_ASSERT_EQUAL_MEMORY_ARRAY(&a, &b, sizeof(struct t), 1);

  a = (struct t){0}; b = (struct t){0};
  a = (struct t){.kind = KIND_FPDP, .data.fpdp = 0.3826 }; 
  size[0] = pack(serbuf, alignof(struct t), "[C]", &tagged_struct_formatter, &a);
  size[1] = unpack(serbuf, alignof(struct t), "[C]", &tagged_struct_formatter, &b);
  TEST_ASSERT_EQUAL_MESSAGE(size[0], size[1], "tagged struct size"); 
  TEST_ASSERT_EQUAL_MEMORY_ARRAY(&a, &b, sizeof(struct t), 1);

  a = (struct t){0}; b = (struct t){0};
  a = (struct t){.kind = KIND_I16, .data.i16 = -7162 }; 
  size[0] = pack(serbuf, alignof(struct t), "[C]", &tagged_struct_formatter, &a);
  size[1] = unpack(serbuf, alignof(struct t), "[C]", &tagged_struct_formatter, &b);
  TEST_ASSERT_EQUAL_MESSAGE(size[0], size[1], "tagged struct size"); 
  TEST_ASSERT_EQUAL_MEMORY_ARRAY(&a, &b, sizeof(struct t), 1);

  a = (struct t){0}; b = (struct t){0};
  a = (struct t){.kind = KIND_U32, .data.u32 = (unsigned int)0xff32 << 16 };
  size[0] = pack(serbuf, alignof(struct t), "[C]", &tagged_struct_formatter, &a);
  size[1] = unpack(serbuf, alignof(struct t), "[C]", &tagged_struct_formatter, &b);
  TEST_ASSERT_EQUAL_MESSAGE(size[0], size[1], "tagged struct size"); 
  TEST_ASSERT_EQUAL_MEMORY_ARRAY(&a, &b, sizeof(struct t), 1);
}*/

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(card_serde_test);
  RUN_TEST(card_stack_serde_test);
  RUN_TEST(board_serde_test);
  return UNITY_END();
}
