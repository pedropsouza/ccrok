#include "solr.h"

size_t
card_serde(pbuf_t *buf, char mode, void *vc) {
  card *c = vc;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(card);
  return pack_unpack(buf, mode, 1, "CCCC", &c->s, &c->r, &c->faceup, &c->clr);
}

size_t
card_stack_serde(pbuf_t *buf, char mode, void *vs) {
  card_stack *s = vs;
  // TODO: VARIABLE LEN
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(card_stack);
  return pack_unpack(buf, mode, 8, "52cCCCCC",
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
  assert(dst->top + n <= 13*4);

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
  for (int i = STACK_SUITS_BEGIN; i <= STACK_SUITS_END; i++) {
    for (int j = 0; j <= STACK_BOARD_END; j++) {
      card_stack *dst = stack_from_id(s, i), *src = stack_from_id(s, j);
      if (!dst->top || !src->top) continue;
      card *a = src->s + (src->top - 1), *b = dst->s + (dst->top - 1);
      if (a->s == b->s && a->r == b->r + 1) {
        return (card_stack_pair){.src = src, .dst = dst};
      }
    }
  }
  fprintf(stderr, "next_ffw_stack failed!\n");
  return (card_stack_pair){0};
}

