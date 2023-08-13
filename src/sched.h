#ifndef SCHED_HG__
#define SCHED_HG__

// A mechanism to control the number of executions per second of a game loop
// Inner state machine driven by the callbacks' returned state transitions,
// tracks external data through the setup and cleanup functions which may
// allocate and deallocate persistent_data.
//
// Cleanup callback is only called if persistent_data != NULL
// Each sched_exec should only call one kind of callback, multiple times
// if running behind schedule (only tick and draw)
//
// Errors can be thrown, and will call the error callback once per throw, upon
// the next exec
//
// Follows justified programming for ease of debugging
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SCHED_TRANS_HISTORY_SIZE 32
#define SCHED_TRANS_QUEUE_SIZE 16
#define SCHED_CALL_MAX_TICKS 32
// forbid overly long schedules when in sched_handover
#define SCHED_SETUP_MAX_EXECS 8
#define SCHED_CLEANUP_MAX_EXECS 8
#define SCHED_MAX_LOOPS 4
// how much time to leave out of the nanosleep, in nanoseconds
#define SCHED_SLEEP_MARGIN 0

enum sched_state {
  SCHED_HALT = 0,
  SCHED_SETUP,
  SCHED_CLEANUP,
  SCHED_CONTINUE,
  SCHED_DROPPED,
  SCHED_PAUSE,
  SCHED_ERR,
};

struct sched_trans {
  enum sched_state s;
  const char *why;
};

struct scheduler;
typedef struct sched_trans (*setup_fun)(struct scheduler *s);
typedef struct sched_trans (*cleanup_fun)(struct scheduler *s);
typedef struct sched_trans (*fixed_fun)(struct scheduler *s, long step_ns);
typedef struct sched_trans (*error_fun)(struct scheduler *s);
typedef void (*render_fun)(struct scheduler *s, double a);

// UB after nanosecond readings of ~2.6 centuries, as per long limits.
struct scheduler {
  long fixedt_ns, rendert_ns, newt_ns, curt_ns, idlet_ns;
  long t[2];
  long acc[2];
  int dropped_last_exec;
  int forced_trans;

  const char *name;
  void *persistent_data; // modified by the callbacks, MAY be set
                         // to something != NULL after setup to make sure
                         // cleanup is called
  void *setup_data; // not modified internally, used only for setup and cleanup
  size_t trans_queue_fidx;
  size_t trans_queue_lidx;
  struct sched_trans trans_queue[SCHED_TRANS_QUEUE_SIZE];
  size_t trans_hist_idx;
  struct sched_trans trans_hist[SCHED_TRANS_HISTORY_SIZE];
  setup_fun setup;
  fixed_fun fixed;
  render_fun render;
  cleanup_fun cleanup;
  error_fun error;
};

// initialize a scheduler.
// both timesteps in nanosecond periods,
// freeing setup_data still left to the caller.
//
// returns the initialized scheduler
struct scheduler sched_init(setup_fun s, cleanup_fun c, fixed_fun f,
                            render_fun r, long fixedstep_ns, long renderstep_ns,
                            void *setup_data, const char *name);
// request a transition.
// there may be many transitions requests per exec(), they are thrown in a
// queue. each transition request ensures  its state will be resolved at some
// point in the future (if no rejection state is resolved before it), but it may
// not be the next exec, and the previous state may not be the same as the
// requesting state. transitions which request for a state already last in queue
// or already being executed are ignored.
//
// returns truthy if transition was queued, falsy if it was ignored
//
// ! handles a maximum of SCHED_TRANS_QUEUE_SIZE queued transitions !
int sched_transition(struct scheduler *s, enum sched_state new_st,
                     const char *why);

// same as sched_transition, but guaranteed to exec next frame.
void sched_force_transition(struct scheduler *s, enum sched_state new_st,
                            const char *why);

// follow the run through to the end, unsupervised.
//
// returns the final state
enum sched_state sched_handover(struct scheduler *s);

// advance execution.
//
// returns the next exec state
enum sched_state sched_exec(struct scheduler *s);

// advance enough to change the next exec state, but exec no more than n.
//
// returns the next exec state
enum sched_state sched_exec_till_change(struct scheduler *s, unsigned int n,
                                        const char *why);

// advance to the specified state, maximum n execs ahead.
//
// returns the achieved state, which could be different than the target
enum sched_state sched_exec_till_state(struct scheduler *s, enum sched_state st,
                                       unsigned int n, const char *why);

// runs up to but not including a clean halt, failing that, an error.
//
// returns a copy of the transition applied.
struct sched_trans sched_exit(struct scheduler *s, const char *why);

// returns the next exec state.
enum sched_state sched_get_state(struct scheduler *s);

// returns the newest requested state, or the next exec state if no transition
// requests have been made.
enum sched_state sched_get_newest_state(struct scheduler *s);

// returns truthy if dropping the scheduler wouldn't leak memory/leave resources
// allocated, falsy otherwise. is falsy after an allocating setup, truthy after
// cleanup.
int sched_isclean(struct scheduler *s);

// sets the optional error callback
void sched_set_error_callbk(struct scheduler *s, error_fun ef);

// prints a report of relevant internals for debugging to the specified stream.
void sched_debugprint(FILE *stream, struct scheduler *s);

// returns a human-readable string representing the given state.
const char *sched_state_name(enum sched_state);

#endif // SCHED_HG__
