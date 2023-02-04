#include "crok/data.h"

// sets up most things, except the networking subsystem
struct crok_data
crok_game_init(struct crok_setup_data sdata) {
   struct crok_data d = {
    .width = sdata.width,
    .height = sdata.height,
    .servermode = sdata.servermode,
    .crok = crok_state_init(),
    .board = crok_board_calc(),
    .ticknum = 0,
    .statesum = 0,
    .my_player_id = 0,
  };

  if (!d.servermode) {
    struct crok_render_data *rdat = &d.renderdata;

    rdat->w = d.width;
    rdat->h = d.height;
    rdat->a = 0.0;
    rdat->servermode = d.servermode;
    SDL_Init(SDL_INIT_VIDEO);
    rdat->win = SDL_CreateWindow("ccrok", SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED, rdat->w, rdat->h, 0);
    rdat->rdr = SDL_CreateRenderer(rdat->win, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(rdat->rdr, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(rdat->rdr, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(rdat->rdr);

    // modified dimensions maybe
    SDL_GetRendererOutputSize(rdat->rdr, &rdat->w, &rdat->h);

    int smaller = (rdat->w > rdat->h) ? rdat->h : rdat->w;
    rdat->board = crok_board_render_init(rdat->rdr, &d.board, rdat->w / 2,
                                       rdat->h / 2, smaller * 0.9);

    if (TTF_Init() < 0) {
      fprintf(stderr, "Font renderer initialization error: %s\n", SDL_GetError());
      // FIXME: crash and burn?
    }

    struct crok_ui_data *ui = &rdat->ui;
    ui->baseline_ptsize = 16; // TODO: make this a setting
    ui->font = TTF_OpenFont("./resources/font.ttf", rdat->ui.baseline_ptsize);

    assert(ui->font != NULL);
    
    ui->clr = (SDL_Color){.r = 0xff, .g = 0xff, .b = 0xff, .a = 0xff};
    int x = rdat->w/16, y = rdat->h/9, ys = y;
    ui->round = crok_ui_new_text(rdat->rdr, &rdat->ui, "round 00");
    ui->round.dst.x = x;
    ui->round.dst.y = y;
    y += ui->round.dst.h;
    ui->gamestage = crok_ui_new_text(rdat->rdr, &rdat->ui, "gamestage unknown");
    ui->gamestage.dst.x = x;
    ui->gamestage.dst.y = y;
    x = rdat->w/2; y = rdat->h/9;
    ui->score = crok_ui_new_text(rdat->rdr, &rdat->ui, "score 0000");
    ui->score.dst.x = x - ui->score.dst.w/2;
    ui->score.dst.y = y;
    memset(rdat->ui.nameplates, 0, sizeof(struct crok_ui_text) * 4);
  }
  return d;
}

void
crok_game_finish(struct crok_data *d) {
  if (!d->servermode) {
    struct crok_render_data *r = &d->renderdata;
    struct crok_ui_data *ui = &r->ui;
    crok_board_render_finish(&d->renderdata.board);
    crok_ui_free_text(&ui->round);
    crok_ui_free_text(&ui->gamestage);
    crok_ui_free_text(&ui->score);
    for (int i = 0; i < 4; i++) {
      if (crok_unit_valid(d->crok.players + i)) {
        crok_ui_free_text(ui->nameplates + i);
      }
    }
    SDL_DestroyRenderer(r->rdr);
    SDL_DestroyWindow(r->win);
    SDL_Quit();
  }
}

struct ivec
crok_screen_to_board(struct crok_render_data *rd, struct fvec v) {
  double ox = ((double)v.x - rd->board.dest.x) / rd->board.diam;
  double oy = ((double)v.y - rd->board.dest.y) / rd->board.diam;
  double fx = ox * CROK_BOARD_DIAM;
  double fy = oy * CROK_BOARD_DIAM;
  return (struct ivec){.x = fx, .y = fy};
}

struct fvec
crok_board_to_screen(struct crok_render_data *rd, struct ivec v) {
  double fx = (double)v.x / CROK_BOARD_DIAM;
  double fy = (double)v.y / CROK_BOARD_DIAM;
  double ox = (fx * rd->board.diam) + rd->board.dest.x;
  double oy = (fy * rd->board.diam) + rd->board.dest.y;
  return (struct fvec){.x = ox, .y = oy};
}

struct crok_ui_text
crok_ui_new_text(SDL_Renderer *rdr, struct crok_ui_data *ui, char *text) {
  SDL_Surface *s = TTF_RenderUTF8_Solid(ui->font, text, ui->clr);
  assert(s != NULL);
  SDL_Texture *t = SDL_CreateTextureFromSurface(rdr, s);
  struct crok_ui_text uit = {.tex = t, .dst.w = s->w, .dst.h = s->h};
  SDL_FreeSurface(s);
  return uit;
}

void
crok_ui_free_text(struct crok_ui_text *t) {
  SDL_DestroyTexture(t->tex);
}

void
crok_ui_render(struct crok_data *d) {
  struct crok_render_data *rd = &d->renderdata;
  struct crok_ui_data *ui = &rd->ui;

  SDL_RenderCopy(rd->rdr, ui->round.tex, NULL, &ui->round.dst);
  SDL_RenderCopy(rd->rdr, ui->gamestage.tex, NULL, &ui->gamestage.dst);
  SDL_RenderCopy(rd->rdr, ui->score.tex, NULL, &ui->score.dst);

  for (int i = 0; i < 4; i++) {
    struct crok_player *p = d->crok.players + i;
    if (crok_unit_valid(p)) {
      struct crok_ui_text *namep = ui->nameplates + i;
      struct fvec spos = crok_board_to_screen(rd, p->pos);
      namep->dst.x = spos.x - namep->dst.w/2.0;
      namep->dst.y = spos.y + namep->dst.h/2.0;
      SDL_RenderCopy(rd->rdr, namep->tex, NULL, &namep->dst);
    }
  }
}
