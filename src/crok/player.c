#include "crok/player.h"

struct crok_player *crok_player_new(struct crok_state *s, int side,
                                    crok_input_producer_fun inputf,
                                    SDL_Texture *cursor_tex,
                                    char name[CROK_NAMELEN_MAX]) {
  // showing off i know how to use ternary operators yes
  struct crok_player p = {0};
  uint8_t sideoff = (side - CROK_PLAYER_TEAM1) * 2;
  int vacant =
      sideoff + ((crok_player_isvalid(&s->players[sideoff + 0]) &&
                  crok_player_isvalid(&s->players[sideoff + 1]))
                     ? -1 // there's no space left in the game
                     : (crok_player_isvalid(&s->players[sideoff + 0]) ? 1 : 0));
  if (vacant < 0) {
    return NULL;
  }
  p.id = vacant;
  memcpy(p.name, name, CROK_NAMELEN_MAX - 1);
  p.name[CROK_NAMELEN_MAX - 1] = '\0';
  p.flags = CROK_PLAYER_VALID;
  p.cursor_tex = cursor_tex;
  p.cursor_rect.w = 48;
  p.cursor_rect.h = 48;
  p.score = 0;
  p.inputf = inputf;
  s->players[vacant] = p;
  return &s->players[vacant];
}

int crok_player_team(struct crok_player *p) {
  return p->flags & (CROK_PLAYER_TEAM1 | CROK_PLAYER_TEAM2);
}

int crok_player_isvalid(struct crok_player *p) {
  return p->id >= 0 && p->flags & CROK_PLAYER_VALID;
}

enum crok_gamestage crok_player_tick(struct crok_player *p,
                                     struct crok_state *s) {
  crok_input_accum(&p->inputctx, p->inputf, p);
  for (size_t i = 0; i <= p->inputctx.buf_last; i++) {
    struct crok_input_packet pckt = p->inputctx.buf[i];
    switch (pckt.kind) {
    case CROK_INPUT_INVALID:
      break;
    case CROK_INPUT_MOUSE:
      p->cursor_rect.x = pckt.mouse.x - p->cursor_rect.w / 2;
      p->cursor_rect.y = pckt.mouse.y - p->cursor_rect.h / 2;
      break;
    case CROK_INPUT_QUIT:
      return CROK_LEAVE;
      break;
    default:
      break;
    }
  }
  crok_input_clean(&p->inputctx);
  return s->gamestage;
}

void crok_player_render(struct crok_player *p[2], SDL_Renderer *r, double a) {
  SDL_RenderCopy(r, p[0]->cursor_tex, NULL, &p[0]->cursor_rect);
}
