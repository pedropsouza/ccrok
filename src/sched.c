#include "sched.h"

void print_timespec(struct timespec *ts, const char *why) {
  printf("%s -> %lds %ldns\n", why, ts->tv_sec, ts->tv_nsec);
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

struct timespec do_periodic_callback(struct periodic_callback *pc) {
  long delta_ns, snooze_ns;
  long new_ns = 0;
  struct timespec new_ts, snooze_ts = {0};
  int sequential_calls = 0;

  clock_gettime(CLOCK_MONOTONIC, &new_ts);
  new_ns = new_ts.tv_sec * (long)1e9 + new_ts.tv_nsec;

  delta_ns = new_ns - pc->cur_ns;
  pc->cur_ns = new_ns;
  pc->acc_ns += delta_ns;

  switch (pc->strat) {
    case PERIODIC_CALLBACK_CATCH_UP:
      while (pc->acc_ns >= pc->period_ns) {
        pc->func(pc->arg);
        pc->acc_ns -= pc->period_ns;
        sequential_calls += 1;
        if (sequential_calls > pc->max_per_tick) {
          break;
        }
      }
      break;

    case PERIODIC_CALLBACK_NOACC:
      pc->func(pc->arg);
      pc->acc_ns = 0;
      break;
    
    default:
      pc->func(pc->arg);
      pc->acc_ns -= pc->period_ns;
      break;
  }

  // new_ts pulls double duty here, careful not to read it too early
  clock_gettime(CLOCK_MONOTONIC, &new_ts);
  new_ns = new_ts.tv_sec * (long)1e9 + new_ts.tv_nsec;
  snooze_ns = pc->period_ns - delta_ns;
  if (snooze_ns > 0) {
    snooze_ts = ns_to_timespec(snooze_ns);
  }

  return snooze_ts;
}

