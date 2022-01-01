#include "crok/gloop.h"

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
  struct fvec spos = crok_board_to_screen(rd, p[0]->pos);
  SDL_SetRenderDrawColor(rd->rdr, 255, 255, 255, 255);
  SDL_RenderDrawCircle(rd->rdr, spos.x, spos.y, rd->w / 200);
}

struct crok_disc crok_disc_init(struct crok_player *owner) {
  struct crok_disc d = {.pb = physbody_zero("disc"), .flags = owner->flags};
  d.pb.pos.x = CROK_BOARD_DIAM / 2;
  d.pb.pos.y = CROK_BOARD_DIAM / 2;
  d.pb.radius = CROK_DISC_DIAM;
  return d;
}

enum crok_gamestage crok_disc_tick(struct crok_disc *d, struct crok_state *s) {
  physbody_tick(&d->pb);
  return s->gamestage;
}

void crok_disc_render(struct crok_disc *d[2], struct crok_render_data *rd) {
  SDL_Rect dest = {0};
  double s = rd->rboard->disc_diam;
  struct fvec spos = crok_board_to_screen(rd, d[0]->pb.pos);
  dest.w = s;
  dest.h = s;
  dest.x = spos.x - (s - 1.0) / 2.0;
  dest.y = spos.y - (s - 1.0) / 2.0;

  SDL_RenderCopy(rd->rdr, rd->rboard->disc_tex, NULL, &dest);
}
