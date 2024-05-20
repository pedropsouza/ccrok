#include "crok/data.h"

// sets up most things, except the networking subsystem
struct crok_data
crok_game_init(struct crok_setup_data *sdata) {
   struct crok_data d = {0};
   d.width = sdata->width;
   d.height = sdata->height;
   d.servermode = sdata->servermode;
   d.crok = crok_state_init();
   d.board = crok_board_calc();
   d.ticknum = 0;
   d.statesum = 0;
   d.my_player_id = 0;
   d.should_reload = 0;

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

struct fvec
crok_screen_to_board(struct crok_render_data *rd, struct fvec v) {
  double radius = rd->board.radius;
  double ix = ((double)v.x - (rd->board.dest.x + radius)) / rd->board.diam;
  double iy = ((double)v.y - (rd->board.dest.y + radius)) / rd->board.diam;
  double bx = ix * CROK_BOARD_DIAM;
  double by = iy * CROK_BOARD_DIAM;
  return (struct fvec){.x = bx, .y = by};
}

struct fvec
crok_board_to_screen(struct crok_render_data *rd, struct fvec v) {
  double radius = rd->board.radius;
  double ix = (double)v.x / CROK_BOARD_DIAM;
  double iy = (double)v.y / CROK_BOARD_DIAM;
  double sx = (ix * rd->board.diam) + rd->board.dest.x + radius;
  double sy = (iy * rd->board.diam) + rd->board.dest.y + radius;
  return (struct fvec){.x = sx, .y = sy};
}

struct crok_ui_text
crok_ui_new_text(SDL_Renderer *rdr, struct crok_ui_data *ui, char *text) {
  SDL_Surface *s = TTF_RenderUTF8_Solid(ui->font, text, ui->clr);
  assert(s != NULL);
  SDL_Texture *t = SDL_CreateTextureFromSurface(rdr, s);
  struct crok_ui_text uit = {.valid = 1, .tex = t, .dst.w = s->w, .dst.h = s->h};
  SDL_FreeSurface(s);
  return uit;
}

void
crok_ui_free_text(struct crok_ui_text *t) {
  if (!t->valid) return;
  SDL_DestroyTexture(t->tex);
  t->valid = 0;
}

void
crok_ui_render_text(SDL_Renderer *rdr, struct crok_ui_text *t) {
  if (t->valid) {
    SDL_RenderCopy(rdr, t->tex, NULL, &t->dst);
  }
}

void
crok_ui_render(struct crok_data *d) {
  struct crok_render_data *rd = &d->renderdata;
  struct crok_ui_data *ui = &rd->ui;

  crok_ui_render_text(rd->rdr, &ui->round);
  crok_ui_render_text(rd->rdr, &ui->gamestage);
  crok_ui_render_text(rd->rdr, &ui->score);
  crok_ui_render_text(rd->rdr, &ui->debug);

  for (int i = 0; i < 4; i++) {
    struct crok_player *p = d->crok.players + i;
    if (crok_unit_valid(p)) {
      struct crok_ui_text *namep = ui->nameplates + i;
      struct fvec spos = crok_board_to_screen(rd, (struct fvec){.x = p->pos.x, .y = p->pos.y});
      namep->dst.x = spos.x - namep->dst.w/2.0;
      namep->dst.y = spos.y + namep->dst.h/2.0;
      crok_ui_render_text(rd->rdr, namep);
    }
  }
}
