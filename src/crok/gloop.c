#include "crok/gloop.h"

struct crok_player crok_player_init(uint16_t flags, char *name) {
  struct crok_player p = {0};
  strncpy(p.name, name, CROK_PLAYER_NAME_MAXLEN - 1);
  p.name[CROK_PLAYER_NAME_MAXLEN - 1] = '\0';
  p.flags = CROK_U_VALID | flags;
  p.score = 0;
  return p;
}

/**
enum crok_gamestage crok_player_tick(struct crok_player *p,
                                     struct crok_state *s) {
  enum crok_gamestage new_gst = s->gamestage;
  crok_net_accum(&p->inputctx, p->inputf, p);
  for (size_t i = 0; i <= p->inputctx.buf_last; i++) {
    struct crok_net_packet pckt = p->inputctx.buf[i];
    switch (pckt.kind) {
    case CROK_NET_INVALID:
      // Should this fail?
      break;
    case CROK_NET_MOUSE:
      p->cursor_rect.x = pckt.mouse.x - p->cursor_rect.w / 2;
      p->cursor_rect.y = pckt.mouse.y - p->cursor_rect.h / 2;
      break;
    case CROK_NET_QUIT:
      new_gst = CROK_LEAVE;
      printf("leaving...\n");
      break;
    default:
      break;
    }
  }
  crok_net_clean(&p->inputctx);
  return new_gst;
}**/

void crok_player_render(struct crok_player *p[2], struct crok_render_data *rd) {
  struct fvec spos = crok_board_to_screen(rd, p[0]->pos);
  if (p[0]->flags & CROK_U_TEAM1) {
    SDL_SetRenderDrawColor(rd->rdr, 0x22, 0x77, 0xf7, 0xff);
  } else {
    SDL_SetRenderDrawColor(rd->rdr, 0xf7, 0xb7, 0x22, 0xff);
  }
  SDL_RenderDrawCircle(rd->rdr, spos.x, spos.y, rd->w / 200);
}

struct crok_disc crok_disc_init(struct crok_player *owner) {
  struct crok_disc d = {.pb = physbody_zero("disc"), .flags = owner->flags};
  d.pb.pos.x = CROK_BOARD_DIAM / 2;
  d.pb.pos.y = CROK_BOARD_DIAM / 2;
  d.pb.radius = CROK_DISC_DIAM;
  return d;
}

void
crok_disc_tick(struct crok_disc *d, struct crok_state *s) {
  physbody_tick(&d->pb);
}

void crok_disc_render(struct crok_disc *d[2], struct crok_render_data *rd) {
  SDL_Rect dest = {0};
  double s = rd->board.disc_diam;
  struct fvec spos = crok_board_to_screen(rd, d[0]->pb.pos);
  dest.w = s;
  dest.h = s;
  dest.x = spos.x - (s - 1.0) / 2.0;
  dest.y = spos.y - (s - 1.0) / 2.0;

  SDL_RenderCopy(rd->rdr, rd->board.disc_tex, NULL, &dest);
}

void
crok_update_nameplates(struct crok_data *d) {
  for (int i = 0; i < 4; i++) {
    struct crok_player *p = d->crok.players + i;
    if (crok_unit_valid(p)) {
      struct crok_ui_text *nplate = d->renderdata.ui.nameplates + i;
      crok_ui_free_text(nplate);
      *nplate = crok_ui_new_text(d->renderdata.rdr, &d->renderdata.ui, p->name);
    }
  }
}
