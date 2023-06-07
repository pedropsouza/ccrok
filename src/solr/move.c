#include "solr.h"

size_t
move_serde(pbuf_t *buf, char mode, void *vm) {
  move *m = vm;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(move);
  return pack_unpack(buf, mode, 1, "CCCC", &m->kind, &m->stacks.src, &m->stacks.dst, &m->num_cards);
}

int
move_valid(solr *s, move *m, char **reason) {
  *reason = "valid";
  if (m->stacks.src > STACK_MAX || m->stacks.dst > STACK_MAX) {
    *reason = "stack oob";
    return 0;
  }
  card_stack *src = stack_from_id(s, m->stacks.src);
  card_stack *dst = stack_from_id(s, m->stacks.dst);

  if (is_locked_stack(src) || is_locked_stack(dst)) {
    *reason = "one or both stacks are locked";
    return 0;
  } 

#define IS_TABLEAU(x) (x >= STACK_TABLEAU_BEGIN && x <= STACK_TABLEAU_END)
#define IS_STOCK(x) (x == STACK_STOCK)
#define IS_WASTE(x) (x == STACK_WASTE)
#define IS_SUITS(x) (x >= STACK_SUITS_BEGIN && x <= STACK_SUITS_END)

  if (m->kind == MOVE_TAKE) {
    if (IS_STOCK(src->id)) return 1;
    if (dst->top > 0) {
      *reason = "taking with nonempty dst";
      return 0;
    }
    if (((ssize_t)src->top) - (ssize_t)m->num_cards < 0) {
      *reason = "not enough cards to take";
      return 0;
    }
    if (IS_SUITS(src->id) || IS_WASTE(src->id))
      return 1;
    if (IS_TABLEAU(src->id)) {
      ssize_t idx = src->top - m->num_cards;
      assert(idx < 4*13 && idx >= 0);
      int r = src->s[idx].faceup;
      if (!r) *reason = "taking an unknown card";
      return r;
    }
  }
  else if (m->kind == MOVE_PUSH) {
    int empty_dst = dst->top == 0;
    card *top_dst = NULL;
    if (!empty_dst) top_dst = &dst->s[dst->top - 1];
    card *bot_src = &src->s[0];
    if (src->top == 0) {
      *reason = "no cards to push";
      return 0;
    }
    if (((ssize_t)dst->top) + m->num_cards > 13*4) {
      *reason = "too many cards";
      return 0;
    }
    if (IS_TABLEAU(dst->id)) {
      if (empty_dst) {
        if (bot_src->r != RANK_K) {
          *reason = "only kings can be placed there";
          return 0;
        }
        return 1;
      }
      bool color = top_dst->clr != bot_src->clr;
      bool rank = top_dst->r == bot_src->r + 1;
      int r = color && rank;
      if (!r) *reason = "against the rules";
      return r;
    }
    if (IS_STOCK(dst->id)) {
      *reason = "push to stock is forbidden";
      return 0;
    }
    if (IS_WASTE(dst->id)) {
      *reason = "push to waste is forbidden";
      return 0;
    }
    if (IS_SUITS(dst->id)) {
      if (empty_dst) {
        if (bot_src->r != RANK_A) {
          *reason = "only aces can be placed there";
          return 0;
        }
        return 1;
      }
      bool number = src->top == 1;
      bool color = top_dst->clr == bot_src->clr;
      bool rank = bot_src->r == top_dst->r + 1;
      bool suit = bot_src->s == top_dst->s;
      int r = number && color && rank && suit;
      if (!r) *reason = "against the rules";
      return r;
    }
  }
  *reason = "invalid move";
  return 0;

#undef IS_TABLEAU
#undef IS_STOCK
#undef IS_SUITS
}