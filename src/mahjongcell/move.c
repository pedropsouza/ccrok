#include "solr.h"

size_t
move_serde(pbuf_t *buf, char mode, void *vm) {
  move *m = vm;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(move);
  switch (m->kind) {
    case MOVE_PUSH:
    case MOVE_TAKE:
      return pack_unpack(buf, mode, 1, "CCCC",
          &m->kind, &m->stacks.src, &m->stacks.dst, &m->num_cards);
    case MOVE_GROUP_DRAGONS:
      return pack_unpack(buf, mode, 1, "C", &m->suit);
  }
}

int is_seq(card_stack *s, size_t depth) {
  int so = 1;
  if (depth < 2) return 1;
  for (int i = 1; so && (i < depth); i++) {
    card *lo = s->s + (s->top - i);
    card *hi = s->s + (s->top - 1 - i);
    so = lo->r != RANK_DRAGON
      && hi->r != RANK_DRAGON
      && lo->r + 1 == hi->r
      && lo->s != hi->s;
  }
  return so;
}

int dragons_reachable(board *b, uint8_t suit) {
  int hitcount = 0;
  // start from zero so the holding
  // areas are included in the search
  // (this excludes only the player hands, really)
  // ((that's not necessarily a good thing, mind))
  for (int i = 0; i < STACK_BOARD_MAX; i++) {
    card_stack *s = b->stacks + i;
    if (s->top <= 0) continue;
    card *top_card = &s->s[s->top - 1];
    if (top_card->r == RANK_DRAGON && top_card->s == suit) {
      hitcount++;
    }
  }
  return hitcount == 4;
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

#define IS_FLOWER(x) (x == STACK_FLOWER)
#define IS_SUITS(x) (x >= STACK_SUITS_BEGIN && x <= STACK_SUITS_END)
#define IS_HOLDING(x) (x >= STACK_HOLDING_1 && x <= STACK_HOLDING_3)
#define IS_GAME_PILE(x) (x >= STACK_BOARD_BEGIN && x <= STACK_BOARD_END)

  switch (m->kind) {
    case MOVE_TAKE: {
      if (dst->top > 0) {
        *reason = "taking with nonempty dst";
        return 0;
      }
      if (((ssize_t)src->top) - (ssize_t)m->num_cards < 0) {
        *reason = "not enough cards to take";
        return 0;
      }
      if (IS_HOLDING(src->id)) {
        if (src->top >= 2) {
          *reason = "tried to take a completed dragon pile";
          return 0;
        }
        if (m->num_cards != 1) {
          *reason = "taking more than one card from holding pile";
          return 0;
        }
        return 1;
      }
      if (IS_SUITS(src->id)) {
        *reason = "taking from suits pile";
        return 0;
      }

      if (!is_seq(src, m->num_cards)) {
        *reason = "taking a non-sequence";
        return 0;
      }
      return 1;
      break;
    };
    case MOVE_PUSH: {
      int empty_dst = dst->top == 0;
      card *top_dst = NULL;
      if (!empty_dst) top_dst = &dst->s[dst->top - 1];
      card *bot_src = &src->s[0];
      if (src->top == 0) {
        *reason = "no cards to push";
        return 0;
      }
      if (((ssize_t)dst->top) + m->num_cards > 10*3) {
        *reason = "too many cards";
        return 0;
      }
      bool is_suits = IS_SUITS(dst->id);
      if (!is_suits) {
        if (IS_HOLDING(dst->id)) {
          if (!empty_dst) {
            *reason = "can only place a card on an empty holding pile";
            return 0;
          }
          if (src->top > 1) {
            *reason = "can only place one card in holding pile";
            return 0;
          }
          return 1;
        }

        if (IS_FLOWER(dst->id) && bot_src->s != SUIT_FLOWER) {
          *reason = "reserved for the flower :3";
          return 0;
        }

        if (!empty_dst) {
          if (top_dst->r == RANK_DRAGON) {
            *reason = "can't put anything on top of a dragon";
            return 0;
          }
          if (top_dst->s == bot_src->s) {
            *reason = "color rule";
            return 0;
          }
          if (top_dst->r != bot_src->r + 1) {
            *reason = "rank rule";
            return 0;
          }
        }
        return 1;
      }

      if (is_suits) {
        if (bot_src->r == RANK_DRAGON || bot_src->s == SUIT_FLOWER) {
          *reason = "suit piles only take number cards";
          return 0;
        }
        if (empty_dst) {
          if (bot_src->r != RANK_1) {
            *reason = "only ones can be placed there";
            return 0;
          }
          return 1;
        }
        bool number = src->top == 1;
        bool color = top_dst->s == bot_src->s;
        bool rank = bot_src->r == top_dst->r + 1;
        bool suit = bot_src->s == top_dst->s;
        int r = number && color && rank && suit;
        if (!r) *reason = "against the rules";
        return r;
      }
      break;
    }
    case MOVE_GROUP_DRAGONS: {
      if (!dragons_reachable(&s->st.board, m->suit)) {
        *reason = "not all dragons are exposed";
        return 0;
      }
      return 1;
      break;
    }
  }
  *reason = "invalid move";
  return 0;

#undef IS_TABLEAU
#undef IS_STOCK
#undef IS_SUITS
}
