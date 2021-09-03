#ifndef SCHED_HG__
#define SCHED_HG__

#include <SDL.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SCHED_TRANS_HISTORY_SIZE 32
#define SCHED_TRANS_QUEUE_SIZE 16
#define SCHED_CALL_MAX_TICKS 32
// how much time to leave out of the nanosleep, in nanoseconds
#define SCHED_SLEEP_MARGIN 0

enum sched_state {
  SCHED_NONE = 0,
  SCHED_SETUP,
  SCHED_CLEANUP,
  SCHED_CONTINUE,
  SCHED_DROPPED,
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
typedef struct sched_trans (*setup_fun)(struct scheduler *s);
typedef struct sched_trans (*cleanup_fun)(struct scheduler *s);
typedef struct sched_trans (*fixed_fun)(struct scheduler *s, long step_ns);
typedef void (*render_fun)(struct scheduler *s, double a);

// UB after ~49 days (SDL_GetTick), but who cares
struct scheduler {
  long fixedt_ns, rendert_ns, newt_ns, curt_ns, idlet_ns;
  long t[2];
  long acc[2];
  int dropped_last_exec;
  void *persistant_data; // modified by the subordinate functions, MUST be set
                         // to something != NULL after setup!
  void *setup_data; // not modified internally, used only for setup and cleanup
  // head always stays one position ahead of tail
  size_t trans_queue_head_idx;
  size_t trans_queue_tail_idx;
  struct sched_trans trans_queue[SCHED_TRANS_QUEUE_SIZE];
  size_t trans_hist_idx;
  struct sched_trans trans_hist[SCHED_TRANS_HISTORY_SIZE];
  setup_fun setup;
  fixed_fun fixed;
  render_fun render;
  cleanup_fun cleanup;
};

// both timesteps in nanosecond periods
struct scheduler sched_init(setup_fun s, fixed_fun f, render_fun r,
                            cleanup_fun c, long fixedstep_ns,
                            long renderstep_ns, void *setup_data);
// returns falsy if transition was ignored for being a noop
// use enqueue for deffering transitions to the next few frames, up to
// SCHED_TRANS_QUEUE_SIZE transitions/frames
int sched_enqueue(struct scheduler *s, struct sched_trans t);

int sched_exec(struct scheduler *s);
enum sched_state sched_exec_till_change(struct scheduler *s);
void sched_exit(struct scheduler *s);
enum sched_state sched_get_cur_state(struct scheduler *s);
enum sched_state sched_get_newest_state(struct scheduler *s);
int sched_isclean(struct scheduler *s);
void sched_debugprint(FILE *stream, struct scheduler *s);
const char *sched_enum_name(enum sched_state);

#endif // SCHED_HG__
