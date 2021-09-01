#ifndef SCHED_HG__
#define SCHED_HG__

#include <stdlib.h>

#define SCHED_TRANS_HISTORY_SIZE 256

enum sched_state {
  SCHED_SETUP,
  SCHED_CLEANUP,
  SCHED_CONTINUE,
  SCHED_PAUSE,
  SCHED_HALT,
  SCHED_ERR,
};

struct sched_trans {
  enum sched_state s;
  const char *why;
};

struct scheduler;
// onus of dealloc on the caller for udata
typedef struct sched_trans (*setup_fun)(struct scheduler *s, void *udata);
typedef struct sched_trans (*cleanup_fun)(struct scheduler *s, void *udata);

typedef struct sched_trans (*fixed_fun)(struct scheduler *s, double rate,
                                        void *udata);
typedef struct sched_trans (*render_fun)(struct scheduler *s, double a,
                                         void *udata);

// UB after ~49 days (SDL_GetTick), but who cares
struct scheduler {
  double t, dt;
  double current_time;
  double accumulator;
  void *persistant_data;
  size_t trans_hist_idx;
  struct sched_trans trans_hist[SCHED_TRANS_HISTORY_SIZE];
  setup_fun setup;
  fixed_fun fixed;
  render_fun render;
  cleanup_fun cleanup;
};

struct scheduler sched_init(setup_fun s, fixed_fun f, render_fun r,
                            cleanup_fun c, double rate);
void sched_change(struct scheduler *s, struct sched_trans t);
int sched_tick(struct scheduler *s);
void sched_debugprint(struct scheduler *s);

#endif // SCHED_HG__
