#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "solr.h"

void gui_init(solr *s) {
  s->nk = nk_sdl_init(s->rdr.win, s->rdr.rdr);
  {
    int font_scale = 1;
    struct nk_font_atlas *atlas;
    struct nk_font_config config = nk_font_config(0);
    struct nk_font *font;

    /* set up the font atlas and add desired font; note that font sizes are
     * multiplied by font_scale to produce better results at higher DPIs */
    nk_sdl_font_stash_begin(&atlas);
    font = nk_font_atlas_add_default(atlas, 13 * font_scale, &config);
    /*font = nk_font_atlas_add_from_file(atlas,
     * "../../../extra_font/DroidSans.ttf", 14 * font_scale, &config);*/
    /*font = nk_font_atlas_add_from_file(atlas,
     * "../../../extra_font/Roboto-Regular.ttf", 16 * font_scale, &config);*/
    /*font = nk_font_atlas_add_from_file(atlas,
     * "../../../extra_font/kenvector_future_thin.ttf", 13 * font_scale,
     * &config);*/
    /*font = nk_font_atlas_add_from_file(atlas,
     * "../../../extra_font/ProggyClean.ttf", 12 * font_scale, &config);*/
    /*font = nk_font_atlas_add_from_file(atlas,
     * "../../../extra_font/ProggyTiny.ttf", 10 * font_scale, &config);*/
    /*font = nk_font_atlas_add_from_file(atlas,
     * "../../../extra_font/Cousine-Regular.ttf", 13 * font_scale, &config);*/
    nk_sdl_font_stash_end();

    /* this hack makes the font appear to be scaled down to the desired
     * size and is only necessary when font_scale > 1 */
    font->handle.height /= font_scale;
    /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
    nk_style_set_font(s->nk, &font->handle);
  }
}

void gui_finish(solr *s) { nk_sdl_shutdown(); }

void gui_update(solr *s) {
  struct nk_context *nk = s->nk;
  if (nk_begin(nk, "Settings", nk_rect(50, 50, 230, 250),
    NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
    NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {

    nk_layout_row_static(nk, 30, 80, 1);
    if (nk_button_label(nk, "Autosolve")) {
      s->net.tx.kind = PCKT_FFWREQ;
      s->net.tx.data_len = 0;
      netmgr_send(&s->net);
    }
    nk_layout_row_dynamic(nk, 30, 2);
    nk_edit_string(nk, NK_EDIT_FIELD|NK_EDIT_GOTO_END_ON_ACTIVATE, (char*)&s->sdata.username, &s->sdata.username_len, SOLR_NAME_LEN, NULL);
  }
  nk_end(nk);
  nk_sdl_render(NK_ANTI_ALIASING_ON);
}
