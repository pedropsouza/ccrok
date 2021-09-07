#include "sched.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

#define RATE 1e9 / 1000
#define MAX_TICKS 1000

struct timespec new_ts;
long timestamps[MAX_TICKS];
int badticks;

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
  struct scheduler s =
      sched_init(test_setup, test_cleanup, test_fixed, test_render, RATE,
                 1e9 / 50, NULL, "ticktest");
  sched_handover(&s);
  for (int i = 0; i < MAX_TICKS - 1; i++) {
    if ((timestamps[i + 1] - timestamps[i]) <= (RATE) &&
        (timestamps[i + 1] - timestamps[i]) >= (RATE)) {
      badticks++;
    }
  }
  fprintf(stderr, "%d", badticks);
  TEST_ASSERT(badticks == 0);
}

int main(void) {
  UNITY_BEGIN();
  badticks = 0;
  RUN_TEST(tickTest);
  return UNITY_END();
}
