#include "crok/data.h"

struct ivec crok_screen_to_board(struct crok_render_data *rd, struct fvec v) {
  double ox = ((double)v.x - rd->rboard->dest.x) / rd->rboard->diam;
  double oy = ((double)v.y - rd->rboard->dest.y) / rd->rboard->diam;
  double fx = ox * CROK_BOARD_DIAM;
  double fy = oy * CROK_BOARD_DIAM;
  return (struct ivec){.x = fx, .y = fy};
}

struct fvec crok_board_to_screen(struct crok_render_data *rd, struct ivec v) {
  double fx = (double)v.x / CROK_BOARD_DIAM;
  double fy = (double)v.y / CROK_BOARD_DIAM;
  double ox = (fx * rd->rboard->diam) + rd->rboard->dest.x;
  double oy = (fy * rd->rboard->diam) + rd->rboard->dest.y;
  return (struct fvec){.x = ox, .y = oy};
}
