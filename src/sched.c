#include "sched.h"

struct scheduler sched_init(setup_fun s, fixed_fun f, render_fun r,
                            cleanup_fun c) {
  struct scheduler sched = {0};
  sched.setup = s;
  sched.fixed = f;
  sched.render = r;
  sched.cleanup = c;
  return sched;
}
void sched_change(struct scheduler *s, struct sched_trans t) {
  size_t new_idx = s->trans_hist_idx + 1;
  if (new_idx >= SCHED_TRANS_HISTORY_SIZE) {
    new_idx = 0;
  }
  s->trans_hist[new_idx] = t;
}
int sched_tick(struct scheduler *s) {}
void sched_debugprint(struct scheduler *s) {}
