#include "sched.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

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
struct sched_trans test_setup(struct scheduler *s) {
  s->persistant_data = "foo";
  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "setup finished"};
}

struct sched_trans test_cleanup(struct scheduler *s) {
  s->persistant_data = NULL;
  return (struct sched_trans){.s = SCHED_HALT, .why = "cleanup finished"};
}

struct sched_trans test_fixed(struct scheduler *s, long step_ns) {
  static long num_ticks = 0;
  if (num_ticks >= MAX_TICKS) {
    // fixed got called after a cleanup request
    extra_fixed_call = 1;
  }
  clock_gettime(CLOCK_MONOTONIC, &new_ts);
  timestamps[num_ticks] = (new_ts.tv_sec * 1e9) + new_ts.tv_nsec;
  num_ticks += 1;

  if (num_ticks >= MAX_TICKS) {
    return (struct sched_trans){.s = SCHED_CLEANUP,
                                .why = "testing array full"};
  }
  return (struct sched_trans){.s = SCHED_CONTINUE, .why = "fixed call"};
}

void test_render(struct scheduler *s, double a) {}

void tickTest() {
  badticks = 0;
  struct scheduler s =
      sched_init(test_setup, test_cleanup, test_fixed, test_render, RATE,
                 1e9 / 50, NULL, "ticktest");
  sched_handover(&s);
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
