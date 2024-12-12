#include "solr.h"

size_t
card_serde(pbuf_t *buf, char mode, void *vc) {
  card *c = vc;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(card);
  return pack_unpack(buf, mode, 1, "CCC", &c->s, &c->r, &c->faceup);
}

size_t
card_stack_serde(pbuf_t *buf, char mode, void *vs) {
  card_stack *s = vs;
  // TODO: VARIABLE LEN
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(card_stack);
  return pack_unpack(buf, mode, 8, "50cCCCCC",
    &s->s, &s->top, &s->splayed, &s->pile, &s->id, &s->lock_info);
}

void
update_stack_dims(solr* s, card_stack *ss) {
  solr_rdr *rd = &s->rdr;
  SDL_Rect *target = rd->stack_areas + ss->id;
  target->w = rd->card_dims.x;
  target->h = rd->card_dims.y;
  if (ss->splayed) {
    target->h += rd->card_dims.y*ss->top/6;
  } else if (ss->pile) {
    //this line just straight up doesn't make any sense
    //target->h -= (2*rd->win_dims.y/ref_window_dims.y)*ss->top;
  }
}

void
shuffle_stack(card_stack *s) {
  if (s->top > 1) {
    size_t i;
    for (i = 0; i < s->top - 1; i++) {
      size_t j = i + rand() / (RAND_MAX / (s->top - i) + 1);
      card t = s->s[j];
      s->s[j] = s->s[i];
      s->s[i] = t;
    }
  }
}

void
push_card_stack(card_stack *dst, card_stack *src) {
  assert(dst != src);
  assert(dst->top + src->top <= 13*4);
  assert(src->top > 0);
  for (ssize_t i = 0; i < src->top; i++) {
    dst->s[dst->top] = src->s[i];
    src->s[i] = (card){0};
    dst->top++;
  }
  src->top = 0;
}
void
take_card_stack(card_stack *src, card_stack *dst, size_t n) {
  assert(dst != src);
  assert(src->top - (ssize_t)n >= 0);
  assert(dst->top + n <= 10*5);

  for (ssize_t i = src->top - n; i < src->top; i++) {
    card c = src->s[i];
    src->s[i] = (card){0};
    dst->s[dst->top] = c;
    dst->top++;
  }
  src->top -= n;
}
void
expose_top_card_stack(card_stack *s) {
  if (s->top == 0) return;
  s->s[s->top - 1].faceup = 1;
}

int
is_locked_stack(card_stack *stack) {
  return stack->lock_info & 0b1;
}
int
who_locked_stack(card_stack *stack) {
  return (stack->lock_info & ~0b1) >> 1;
  // max 127 players :)
}

void
lock_stack(card_stack *stack, uint8_t who) {
  stack->lock_info = 0b1;
  stack->lock_info |= who << 1;
}
void
unlock_stack(card_stack *stack) {
  stack->lock_info &= ~0b1;
}

card_stack*
stack_from_id(solr *s, uint8_t id) {
  assert(id < STACK_MAX);
  return id >= STACK_PLAYER_BEGIN ? &s->st.players[id - STACK_PLAYER_BEGIN].held.stack :
    s->st.board.stacks + id;
}

card_stack_pair
next_ffw_stacks(solr *s) {
  card_stack_pair *cpair = &s->srv.ffwd_able_stacks;
  if (cpair->src != NULL && cpair->dst != NULL) {
    move m = {
      .kind = MOVE_PUSH,
      .stacks.src = cpair->src->id,
      .stacks.dst = cpair->dst->id,
      .num_cards = 1
    };
    char *reason;
    if (move_valid(s, &m, &reason)) return *cpair;
    else { *cpair = (card_stack_pair){0}; }
  }
  for (size_t i = STACK_BOARD_BEGIN; i <= STACK_BOARD_END; i++) {
    card_stack *srcs = stack_from_id(s, i);
    if (srcs->top == 0) { continue; }
    int top_rank = srcs->s[srcs->top-1].r;
    int top_suit = srcs->s[srcs->top-1].s;
    if (top_rank == RANK_DRAGON) { continue; }
    if (top_suit == SUIT_FLOWER) { continue; }
    
    int seq_d = stack_sequence_depth(srcs);
    if (seq_d) {
      card_stack *dsts = NULL;
      for (int i = STACK_SUITS_BEGIN; i <= STACK_SUITS_END; i++) {
        card_stack *ss = stack_from_id(s, i);
        move m = {
          .kind = MOVE_PUSH,
          .stacks.src = srcs->id,
          .stacks.dst = ss->id,
          .num_cards = 1
        };
        printf("%s:\n test %s\n", __func__, move_repr(s, &m));
        char *reason;
        if (move_valid(s, &m, &reason)) {
          dsts = stack_from_id(s, ss->id);
          printf("%s: move found!\n", __func__);
          break; // validity check / dest. search loop
        }
      } // end for all possible destinations
      // invalid move check
      if (dsts == NULL) continue; // for all stacks in board

      // conservative anti-softlock check
      int possibly_needed = 0;
      if (top_rank != 1) { // 1's are never needed elsewhere
        for (int i = STACK_BOARD_BEGIN; i <= STACK_BOARD_END; i++) {
          card_stack *os = stack_from_id(s, i);
          for (int j = 0; j < os->top; j++) {
            if (os->s[j].r == top_rank - 1) {
              printf("%s: determined to be possible softlock!\n", __func__);
              possibly_needed = 1;
              break;
            }
          }
          if (possibly_needed) break;
        } // end anti softlock check loop
        if (possibly_needed) {
          continue; // for all stacks in board
        }
      } // end if source top rank is not 1
      // cache
      *cpair = (card_stack_pair){.src = srcs, .dst = dsts};
      return *cpair;
    } // end if stack is a non-zero sequence
  } // end for all stacks in board

  fprintf(stderr, "next_ffw_stack failed!\n");
  card_stack_pair zeroed_pair = {0};
  *cpair = zeroed_pair;
  return *cpair;
}

