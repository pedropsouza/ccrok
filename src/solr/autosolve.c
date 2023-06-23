#include "ivec.h"
#include "solr.h"
#include <math.h>
#include <stdint.h>

size_t
ffw_anim_serde(pbuf_t *buf, char mode, void *van) {
  ffw_anim *an = van;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(board);
  return pack_unpack(buf, mode, 8, "QII", &an->lerpval, &an->src, &an->dst);
}

int
is_ffw_able(solr *s) {
  for (size_t i = 0; i < STACK_MAX; i++) {
    card_stack *cs = stack_from_id(s, i);
    if (cs->top == 0) { continue; }
    for (size_t j = 0; j < cs->top; j++) {
      if(!cs->s[j].faceup) {
        //printf("got face down card @ %zu,%zu\n", i,j);
        return 0;
      }
    }
  }
  return 1;
}

void
ffw(solr *s) {
  assert(is_ffw_able(s));
  s->st.stage = STAGE_FFW;
  player *bot = s->st.players + SOLR_AUTO_PLAYER;
  bot->valid = 1;
  s->net.tx.kind = PCKT_STDUMP;
  s->net.tx.data_len = pack(s->net.tx.data, 8, "G", &s->st);
  broadcast(s);
}

// Only sends the move and synchronized easing value
void
ffw_animate_server(solr *s) {
  solr_rdr *rd = &s->rdr;
  static card_stack_pair anim_stacks = {0};
  static float anim_lerp = 0.0f;
  if (!s->sdata.server) return;

  player *bot = s->st.players + SOLR_AUTO_PLAYER;
  assert(bot->valid);
  move m;
  if (bot->held.stack.top) {
    if (anim_lerp > 0.99) {
      m = (move){
        .kind = MOVE_PUSH,
        .stacks.src = STACK_PLAYER_BEGIN + SOLR_AUTO_PLAYER,
        .stacks.dst = anim_stacks.dst->id,
        .num_cards = 1,
      };

      s->net.tx.kind = PCKT_MOVE;
      s->net.tx.data_len = pack(s->net.tx.data, 1, "m", &m);
      handle_pckt(s, &s->net.tx);
      anim_lerp = 0;
    } else {
      float ease = 1-powf(1-anim_lerp, 3);
      ffw_anim an = {
        .lerpval = (uint64_t)(anim_lerp * (float)UINT64_MAX),
        .src = anim_stacks.src->id, .dst = anim_stacks.dst->id
      };

      s->net.tx.kind = PCKT_FFWANIM;
      s->net.tx.data_len = pack(s->net.tx.data, 1, "a", &an);
      broadcast(s);
      anim_lerp += 0.05;
    }
  } else {
    anim_stacks = next_ffw_stacks(s);
    if (anim_stacks.src == NULL || anim_stacks.dst == NULL) {
      printf("ffw failed, returning to play stage\n");
      s->st.stage = STAGE_PLAYING;
      bot->valid = 0;
      return;
    }
    m = (move){
      .kind = MOVE_TAKE,
      .stacks.src = anim_stacks.src->id,
      .stacks.dst = STACK_PLAYER_BEGIN + SOLR_AUTO_PLAYER,
      .num_cards = 1,
    };

    s->net.tx.kind = PCKT_MOVE;
    s->net.tx.data_len = pack(s->net.tx.data, 1, "m", &m);
    handle_pckt(s, &s->net.tx);
  }
}

void
ffw_animate_client(solr *s, ffw_anim *an) {
  solr_rdr *rd = &s->rdr;
  if (s->sdata.server) return;

  player *bot = s->st.players + SOLR_AUTO_PLAYER;
  assert(bot->valid);
  if (bot->held.stack.top) {
    struct ivec srcpos = {
      .x = rd->stack_areas[an->src].x,
      .y = rd->stack_areas[an->src].y + yoff_card_stack(&s->rdr, stack_from_id(s, an->src)),
    };
    struct ivec dstpos = {
      .x = rd->stack_areas[an->dst].x,
      .y = rd->stack_areas[an->dst].y
    };
    float ease = 1-powf(1-((float)an->lerpval/(float)UINT64_MAX), 3);
    struct ivec pos = {
      .x = (int64_t)(lerp(srcpos.x, dstpos.x, ease)),
      .y = (int64_t)(lerp(srcpos.y, dstpos.y, ease)),
    };
    pos = ivec_add(pos, ivec_div(s->rdr.card_dims, (ivec){2,2}));

    s->net.tx.kind = PCKT_POS;
    char uid = SOLR_AUTO_PLAYER;
    s->net.tx.data_len = pack(s->net.tx.data, 8, "QQC", &pos.x, &pos.y, &uid);
    handle_pckt(s, &s->net.tx);
  }
}
