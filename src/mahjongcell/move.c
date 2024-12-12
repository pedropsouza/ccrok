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

      if (m->num_cards > stack_sequence_depth(src)) {
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
      // pivot is the card which would follow the destination top in
      // a sequence if the move is followed through
      ssize_t pivot_idx = (ssize_t)src->top - m->num_cards;
      if (pivot_idx < 0) {
        *reason = "too few cards in source";
        return 0;
      }
      card *pivot = &src->s[pivot_idx];
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

        if (IS_FLOWER(dst->id) && pivot->s != SUIT_FLOWER) {
          *reason = "reserved for the flower :3";
          return 0;
        }

        if (!empty_dst) {
          if (top_dst->r == RANK_DRAGON) {
            *reason = "can't put anything on top of a dragon";
            return 0;
          }
          if (top_dst->s == pivot->s) {
            *reason = "color rule";
            return 0;
          }
          if (top_dst->r != pivot->r + 1) {
            *reason = "rank rule";
            return 0;
          }
        }
        return 1;
      }

      if (is_suits) {
        if (pivot->r == RANK_DRAGON || pivot->s == SUIT_FLOWER) {
          *reason = "suit piles only take number cards";
          return 0;
        }
        if (empty_dst) {
          if (pivot->r != RANK_1) {
            *reason = "only ones can be placed there";
            return 0;
          }
          return 1;
        }
        bool number = m->num_cards == 1;
        bool color = top_dst->s == pivot->s;
        bool rank = pivot->r == top_dst->r + 1;
        bool suit = pivot->s == top_dst->s;
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

char *
move_repr(solr *s, move *m) {
  static char buf[128];
  char *reason;
  char *name_src = stack_name_from_id(m->stacks.src);
  char *name_dst = stack_name_from_id(m->stacks.dst);
  if (m->stacks.src == NULL || m->stacks.dst == NULL) return "invalid move (null)";
  char valid = move_valid(s, m, &reason);
  
  // pivot is the card which would follow the destination top in
  // a sequence if the move is followed through
  card_stack *src = stack_from_id(s, m->stacks.src);
  ssize_t pivot_idx = (ssize_t)src->top - m->num_cards;
  size_t written = snprintf(buf, 128,
      "%s %s to %s (%d cards):\n%s",
      (m->kind == MOVE_TAKE)? "taking"
        : (m->kind == MOVE_PUSH)? "pulling"
        : (m->kind == MOVE_GROUP_DRAGONS)? "grouping dragons"
        : "unknown movement from",
      name_src, name_dst, m->num_cards, reason
  );

  if (pivot_idx >= 0) {
    card *pivot = &src->s[pivot_idx];
    snprintf(buf + written, 128 - written,
        " (pivot %d suit %d)",
        pivot->r, pivot->s
    );
  } else snprintf(buf + written, 128 - written, "(no pivot)");
  return buf;
}
