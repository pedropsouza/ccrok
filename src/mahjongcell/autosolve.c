#include "ivec.h"
#include "solr.h"
#include <math.h>
#include <stdint.h>
#include "mahjcell_net.h"

int // this sneaky lil shit caches the found stack for later opts. alreight :3
is_ffw_able(solr *s) {
  next_ffw_stacks(s);
  card_stack_pair *cpair = &s->srv.ffwd_able_stacks;
  
  // won't ffwd if players are holding cards
  for (size_t i = 0; i < SOLR_MAX_PLAYERS; i++) {
    if (s->st.players[i].held.stack.top) return false;
  }
  return cpair->src != NULL && cpair->dst != NULL;
}

void
ffw(solr *s) {
  assert(is_ffw_able(s));
  s->st.stage = STAGE_FFW;
  player *bot = s->st.players + SOLR_AUTO_PLAYER;
  bot->valid = 1;
  mahjcell_broadcast(s, PCKT_STDUMP);
}

// Only sends the move and synchronized easing value
void
ffw_animate_server(solr *s) {
  solr_rdr *rd = &s->rdr;
  static card_stack_pair anim_stacks = {0};
  static float anim_lerp = 0.0f;
  if (!s->sdata.server) return;

  s->net.tx->tick = s->st.tick;

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

      s->net.tx->kind = PCKT_MOVE;
      s->net.tx->data_len = pack(s->net.tx->data, 1, "[C]", &move_serde, &m);
      handle_pckt(s, s->net.tx);
      anim_lerp = 0.0;
    } else {
      float ease = 1-powf(1-anim_lerp, 3);

      struct fvec srcpos = {
        .x = ref_stack_areas[anim_stacks.src->id].x/ref_window_dims.x,
        .y = ref_stack_areas[anim_stacks.src->id].y/ref_window_dims.y,
      };
      struct fvec dstpos = {
        .x = ref_stack_areas[anim_stacks.dst->id].x/ref_window_dims.x,
        .y = ref_stack_areas[anim_stacks.dst->id].y/ref_window_dims.y,
      };
      struct fvec pos = {
        .x = lerp(srcpos.x, dstpos.x, ease),
        .y = lerp(srcpos.y, dstpos.y, ease),
      };
      
      int8_t id = SOLR_AUTO_PLAYER;
      s->net.tx->kind = PCKT_POS;
      s->net.tx->data_len = pack(s->net.tx->data, 8, "IIC", &pos.x, &pos.y, &id);
      handle_pckt(s, s->net.tx);
      // FIXME: this should be a function of TPS
      const float anim_length_ns = 2.0 * 1e9;
      anim_lerp += s->sdata.frametime_ns/anim_length_ns;
    }
  } else {
    anim_stacks = next_ffw_stacks(s);
    if (anim_stacks.src == NULL || anim_stacks.dst == NULL) {
      printf("ffw finished, returning to play stage\n");
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

    // FIXME: this caching is jank, it should be transparent
    s->srv.ffwd_able_stacks = (card_stack_pair){0};

    s->net.tx->kind = PCKT_MOVE;
    s->net.tx->data_len = pack(s->net.tx->data, 1, "[C]", &move_serde, &m);
    handle_pckt(s, s->net.tx);
  }
}

