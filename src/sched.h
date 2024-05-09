#ifndef SCHED_HG__
#define SCHED_HG__

//#include "solr.h"
#include <time.h>
#include <stdio.h>


enum periodic_callback_strategy {
  PERIODIC_CALLBACK_DEFAULT = 0, // uses accumulator to reduce drift
  PERIODIC_CALLBACK_CATCH_UP, // keep calling until up to speed, but not exceeding `max_per_tick` number of calls
  PERIODIC_CALLBACK_NOACC, // naive approach with no accumulator
};
struct periodic_callback {  
  long cur_ns,
       acc_ns,
       period_ns;
  int max_per_tick;
  enum periodic_callback_strategy strat;
  void *arg;
  void (*func)(void*);
};

struct timespec do_periodic_callback(struct periodic_callback *pc);

void print_timespec(struct timespec *ts, const char *why);
struct timespec ns_to_timespec(long ns);

#endif // STM_HG__
