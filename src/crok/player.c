#include "crok/player.h"

struct crok_player crok_player_init(int side, int team_num, char *name) {
  struct crok_player p = {0};
  memcpy(p.name, name, strnlen(name, CROK_NAMELEN_MAX - 1));
  p.team_num = team_num;
  p.name[CROK_NAMELEN_MAX - 1] = '\0';
  p.flags = CROK_U_VALID | side;
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
