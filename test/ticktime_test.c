#include "sched.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define RATE (1e9 / 1000)
#define THRESHOLD_NS RATE
#define MAX_TICKS 250

struct timespec new_ts;
char extra_fixed_call = 0;
int badticks;
long timestamps[MAX_TICKS];
long padding[32];

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

int ticking_done = 0;
void tick_func(void * _) {
  static long num_ticks = 0;
  if (num_ticks >= MAX_TICKS) {
    // fixed got called after a cleanup request
    extra_fixed_call = 1;
  }
  clock_gettime(CLOCK_MONOTONIC, &new_ts);
  timestamps[num_ticks] = (new_ts.tv_sec * 1e9) + new_ts.tv_nsec;
  num_ticks += 1;

  ticking_done = num_ticks >= MAX_TICKS;
}

void tickTest() {
  badticks = 0;
  struct periodic_callback tick_call = {0};
  tick_call.max_per_tick = 1;
  tick_call.func = tick_func;
  tick_call.strat = PERIODIC_CALLBACK_DEFAULT;
  
  do { do_periodic_callback(&tick_call); } while(!ticking_done);

  long min = LONG_MAX, max = 0;
  int min_tick = -1, max_tick = -1;
  for (int i = 0; i < MAX_TICKS - 1; i++) {
    long delta = timestamps[i + 1] - timestamps[i];
    if (delta > max) {
      max = delta;
      max_tick = i + 1;
    }
    if (delta < min) {
      min = delta;
      min_tick = i + 1;
    }
    if (delta < ((long)RATE - THRESHOLD_NS) ||
        delta > ((long)RATE + THRESHOLD_NS)) {
      badticks++;
    }
  }
  if (extra_fixed_call) {
    fprintf(stderr, "!! extra calls to fixed tick function! !!\n");
  }
  fprintf(stderr,
          "%d/%d ticks out of step\nmin %ldns @ %d, max %ldns @ %d, "
          "fixed delta %ldns\n",
          badticks, MAX_TICKS, min, min_tick, max, max_tick, (long)RATE);
  TEST_ASSERT(badticks == 0);
  TEST_ASSERT(!extra_fixed_call);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(tickTest);
  return UNITY_END();
}
