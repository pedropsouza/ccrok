#include "sched.h"

void print_timespec(struct timespec *ts, const char *why) {
  printf("%s -> %lds %ldns\n", why, ts->tv_sec, ts->tv_nsec);
}

void sched_push_hist(struct scheduler *s, struct sched_trans t) {
  size_t hist = (s->trans_hist_idx + 1) % SCHED_TRANS_HISTORY_SIZE;
  s->trans_hist[hist] = t;
  s->trans_hist_idx = hist;
}

struct sched_trans sched_advance_queue(struct scheduler *s) {
  size_t last = s->trans_queue_lidx;
  size_t first = s->trans_queue_fidx;
  struct sched_trans first_trans = s->trans_queue[first];
  if (first_trans.s != SCHED_NONE) {
    // consumed slots must be reset to SCHED_NONE
    s->trans_queue[first] = (struct sched_trans){0};

    size_t next = (first + 1) % SCHED_TRANS_QUEUE_SIZE;
    if (last == first) {
      last = next;
    }
    first = next;

    s->trans_queue_lidx = last;
    s->trans_queue_fidx = first;
  }
  return first_trans;
}

struct timespec ns_to_timespec(long ns) {
  const long fullsec = (long)1e9;
  struct timespec ts = {0};
  while (ns >= fullsec) {
    ts.tv_sec += 1;
    ns -= fullsec;
  }
  ts.tv_nsec = ns;
  return ts;
}

struct scheduler sched_init(setup_fun s, cleanup_fun c, fixed_fun f,
                            render_fun r, long fixedstep_ns, long renderstep_ns,
                            void *setup_data, const char *name) {
  struct scheduler sched = {0};
  sched.name = name;
  sched.setup = s;
  sched.fixed = f;
  sched.render = r;
  sched.cleanup = c;
  sched.setup_data = setup_data;

  sched.fixedt_ns = fixedstep_ns;
  sched.rendert_ns = renderstep_ns;
  sched.idlet_ns =
      ((fixedstep_ns > renderstep_ns) ? renderstep_ns : fixedstep_ns) -
      SCHED_SLEEP_MARGIN;

  // ensure a render on the first execution, and ignore that time advance
  sched.t[1] = -renderstep_ns;
  sched.acc[1] = renderstep_ns;

  sched_force_transition(&sched, SCHED_SETUP, "sched_init call");
  sched_exec_till_change(&sched, SCHED_SETUP_MAX_EXECS, "sched_init call");
  assert(!sched_isclean(&sched));
  return sched;
}

void sched_handover(struct scheduler *s) {
  while (sched_exec(s))
    ;
}

int sched_transition(struct scheduler *s, enum sched_state new_st,
                     const char *why) {
  if (sched_get_newest_state(s) != new_st) {
    size_t last = s->trans_queue_lidx;
    size_t first = s->trans_queue_fidx;
    size_t target = last;
    const size_t max_s = SCHED_TRANS_QUEUE_SIZE;
    if (s->trans_queue[last].s != SCHED_NONE) {
      target = last + 1 % max_s;
      if (target == first) {
        // queue overflow!
        fprintf(stderr, "scheduler %s: trans_queue overflow!", s->name);
        sched_force_transition(s, SCHED_ERR, "trans_queue overflow!");
        return 0;
      }
    }
    s->trans_queue[target] = (struct sched_trans){.s = new_st, .why = why};
    s->trans_queue_lidx = target;
    return 1;
  }
  return 0;
}

void sched_force_transition(struct scheduler *s, enum sched_state new_st,
                            const char *why) {
  sched_push_hist(s, (struct sched_trans){.s = new_st, .why = why});
  s->forced_trans = 1;
}

int sched_exec(struct scheduler *s) {
  long framet_ns, snooze_ns;
  struct timespec new_ts, snooze_ts;
  int ticks_this_call = 0;
  enum sched_state cur_st = sched_get_cur_state(s);

  switch (cur_st) {
  case SCHED_NONE:
    sched_force_transition(s, SCHED_ERR, "tried to exec with no state!");
    break;
  case SCHED_SETUP: {
    struct sched_trans t = s->setup(s);
    sched_force_transition(s, t.s, t.why);
    clock_gettime(CLOCK_MONOTONIC, &new_ts);
    s->curt_ns = new_ts.tv_sec * (long)1e9 + new_ts.tv_nsec;
  } break;
  case SCHED_CLEANUP: {
    struct sched_trans t = s->cleanup(s);
    sched_force_transition(s, t.s, t.why);
  } break;

  case SCHED_CONTINUE:
    // TODO FIXME: implement state interpolation
    clock_gettime(CLOCK_MONOTONIC, &new_ts);
    s->newt_ns = new_ts.tv_sec * (long)1e9 + new_ts.tv_nsec;

    framet_ns = s->newt_ns - s->curt_ns;
    s->curt_ns = s->newt_ns;

    s->acc[0] += framet_ns;
    s->acc[1] += framet_ns;

    struct sched_trans t = {0};
    while (s->acc[0] >= s->fixedt_ns) {
      t = s->fixed(s, s->fixedt_ns);
      sched_transition(s, t.s, t.why);
      s->t[0] += s->fixedt_ns;
      s->acc[0] -= s->fixedt_ns;
      ticks_this_call += 1;
      if (ticks_this_call >= SCHED_CALL_MAX_TICKS) {
        sched_force_transition(s, SCHED_DROPPED, "call max ticks exceeded!");
        s->dropped_last_exec = s->acc[0] / s->fixedt_ns;
        s->acc[0] = s->acc[0] % s->fixedt_ns;
      }
    }

    // only render once, multiple renders of the same state are equal
    if (s->acc[1] >= s->rendert_ns) {
      s->render(s, 0.0);
      s->acc[1] = s->acc[1] % s->rendert_ns;
      s->t[1] += s->rendert_ns;
    }

    if (ticks_this_call > SCHED_TRANS_QUEUE_SIZE) {
      printf("warning, %d/%d ticks done, possible transition queue overflow!\n",
             ticks_this_call, SCHED_TRANS_QUEUE_SIZE);
    }

    // new_ts pulls double duty here, careful not to read it too early
    clock_gettime(CLOCK_MONOTONIC, &new_ts);
    s->newt_ns = new_ts.tv_sec * (long)1e9 + new_ts.tv_nsec;
    snooze_ns = (s->curt_ns + s->idlet_ns) - s->newt_ns;
    if (snooze_ns > 0) {
      snooze_ts = ns_to_timespec(snooze_ns);
      if (nanosleep(&snooze_ts, NULL) == -1 && errno == EINVAL) {
        sched_force_transition(s, SCHED_ERR, "invalid nanosleep!");
      }
    }
    break;
  case SCHED_DROPPED:
    fprintf(stderr, "scheduler %s: dropped %d fixed ticks!\n", s->name,
            s->dropped_last_exec);
    s->dropped_last_exec = 0;
    sched_force_transition(s, SCHED_CONTINUE, "ignore dropped ticks");

    break;
  case SCHED_PAUSE:
    // noop, wait for external state change
    break;

  case SCHED_HALT:
    if (sched_isclean(s)) {
      return 0; // no further ticks required
    } else {
      sched_force_transition(s, SCHED_ERR,
                             "tried to halt without cleaning up first!");
    }
    break;
  case SCHED_ERR:
    fprintf(stderr, "scheduler %s: SCHED_ERR reached!\n", s->name);
    sched_debugprint(stderr, s);
    abort();
    break;
  }

  if (!s->forced_trans) {
    // consume enqueued transitions from oldest to newest
    struct sched_trans t = sched_advance_queue(s);
    if (t.s != SCHED_NONE) {
      sched_push_hist(s, t);
    }
  } else {
    s->forced_trans = 0;
  }

#ifdef SCHED_TICK_DEBUG
  printf("[%s %.3f,%.3f : %.3f] ticks %d ; fixedt = %.3f/%.3f ; rendert = "
         "%.3f/%.3f\n",
         sched_state_name(cur_st), s->t[0] / 1e9, s->t[1] / 1e9,
         snooze_ns / 1e9, ticks_this_call, s->acc[0] / 1e9, s->fixedt_ns / 1e9,
         s->acc[1] / 1e9, s->rendert_ns / 1e9);
#endif

  return 1; // more ticks are necessary
}

enum sched_state sched_exec_till_change(struct scheduler *s, unsigned int n,
                                        const char *why) {
  enum sched_state old, new;
  old = new = sched_get_cur_state(s);
  for (int i = 0; i < n; i++) {
    sched_exec(s);
    new = sched_get_cur_state(s);
    if (new != old) {
      return new;
    }
  }
  fprintf(stderr,
          "scheduler %s: failed to change from state %s in less than %d "
          "executions! "
          "called with why: \"%s\"; aborting.\n",
          s->name, sched_state_name(old), n, why);
  abort();
}

void sched_exec_till_state(struct scheduler *s, enum sched_state st,
                           unsigned int n, const char *why) {
  enum sched_state old = sched_get_newest_state(s);
  for (int i = 0; i < n; i++) {
    sched_exec(s);
    if (sched_get_cur_state(s) == st) {
      return;
    }
  }
  fprintf(stderr,
          "scheduler %s: failed to reach state %s from state %s in less than "
          "%d executions! "
          "called with why: \"%s\"; aborting.\n",
          s->name, sched_state_name(st), sched_state_name(old), n, why);
  abort();
}

struct sched_trans sched_exit(struct scheduler *s, const char *why) {
  sched_force_transition(s, SCHED_CLEANUP, why);
  sched_exec_till_state(s, SCHED_HALT, SCHED_CLEANUP_MAX_EXECS, why);
  return s->trans_hist[s->trans_hist_idx];
}

enum sched_state sched_get_cur_state(struct scheduler *s) {
  return s->trans_hist[s->trans_hist_idx].s;
}

enum sched_state sched_get_newest_state(struct scheduler *s) {
  enum sched_state queued = s->trans_queue[s->trans_queue_lidx].s;
  if (queued == SCHED_NONE) {
    return sched_get_cur_state(s);
  }
  return queued;
}

int sched_isclean(struct scheduler *s) { return (s->persistant_data == NULL); }

// TODO: implement only printing up to tail
void sched_print_circular_buffer(FILE *stream, struct sched_trans *buf,
                                 size_t head, const size_t cap) {
  for (int i = 0; i < cap; i++) {
    size_t idx = (cap + head - i) % cap;
    struct sched_trans t = buf[idx];
    if (t.s != SCHED_NONE) {
      fprintf(stream, "\t#%d - %s reason \"%s\"\n", i, sched_state_name(t.s),
              t.why);
    }
  }
}

void sched_debugprint(FILE *stream, struct scheduler *s) {
  fprintf(stream, "scheduler %s\n", s->name);
  fprintf(stream, "transition queue:\n");
  sched_print_circular_buffer(stream, s->trans_queue, s->trans_queue_lidx,
                              SCHED_TRANS_QUEUE_SIZE);

  fprintf(stream, "transition history:\n");
  sched_print_circular_buffer(stream, s->trans_hist, s->trans_hist_idx,
                              SCHED_TRANS_HISTORY_SIZE);
}

const char *sched_state_name(enum sched_state st) {
  // TODO maybe transform into a macro?
  switch (st) {
  case SCHED_NONE:
    return "NONE";
  case SCHED_SETUP:
    return "SETUP";
  case SCHED_CLEANUP:
    return "CLEANUP";
  case SCHED_CONTINUE:
    return "CONTINUE";
  case SCHED_DROPPED:
    return "DROPPED";
  case SCHED_PAUSE:
    return "PAUSE";
  case SCHED_HALT:
    return "HALT";
  case SCHED_ERR:
    return "ERR";
  }
}
