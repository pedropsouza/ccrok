#include "crok/player.h"

struct crok_player crok_player_init(uint16_t flags, char *name) {
  struct crok_player p = {0};
  strncpy(p.name, name, CROK_NAMELEN_MAX - 1);
  p.name[CROK_NAMELEN_MAX - 1] = '\0';
  p.flags = CROK_U_VALID | flags;
  p.score = 0;
  return p;
}

/**
enum crok_gamestage crok_player_tick(struct crok_player *p,
                                     struct crok_state *s) {
  enum crok_gamestage new_gst = s->gamestage;
  crok_input_accum(&p->inputctx, p->inputf, p);
  for (size_t i = 0; i <= p->inputctx.buf_last; i++) {
    struct crok_input_packet pckt = p->inputctx.buf[i];
    switch (pckt.kind) {
    case CROK_INPUT_INVALID:
      // Should this fail?
      break;
    case CROK_INPUT_MOUSE:
      p->cursor_rect.x = pckt.mouse.x - p->cursor_rect.w / 2;
      p->cursor_rect.y = pckt.mouse.y - p->cursor_rect.h / 2;
      break;
    case CROK_INPUT_QUIT:
      new_gst = CROK_LEAVE;
      printf("leaving...\n");
      break;
    default:
      break;
    }
  }
  crok_input_clean(&p->inputctx);
  return new_gst;
}**/

void crok_player_render(struct crok_player *p[2], struct crok_render_data *rd) {
}

char *crok_player_serialize(const struct crok_player *p) {
  static char buf[CROK_PLAYER_SERLEN];
  char *w_head = buf;
  // Assuming the whole enum fits into a short uint
  *((uint16_t *)w_head) = htons((uint16_t)p->flags);
  w_head += 2;
  memcpy(w_head, ivec_serialize(&p->pos), IVEC_SERLEN);
  w_head += IVEC_SERLEN;
  *((uint32_t *)w_head) = htonl((uint32_t)p->score);
  w_head += 4;
  strncpy(w_head, p->name, CROK_NAMELEN_MAX);
  return buf;
}

struct crok_player crok_player_deserialize(char buf[CROK_PLAYER_SERLEN]) {
  struct crok_player p;

  char *r_head = buf;
  p.flags = ntohs(*((uint16_t *)r_head));
  r_head += 2;
  p.pos = ivec_deserialize(r_head);
  r_head += IVEC_SERLEN;
  p.score = ntohl(*((uint32_t *)r_head));
  r_head += 4;
  strncpy(p.name, r_head, CROK_NAMELEN_MAX);
  return p;
}
