#include "sched.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

void setUp(void) {}
void tearDown(void) {}

struct sched_trans setup_countdown(struct scheduler *s) {
  int *c = (int *)s->setup_data;
  if (c[0] > 0) {
    c[0] = c[0] - 1;
    return (struct sched_trans){.s = SCHED_SETUP, .why = "counter"};
  } else {
    s->persistent_data = (void *)0xdeadbeef;
    return (struct sched_trans){.s = SCHED_CONTINUE, .why = "counter done"};
  }
}

struct sched_trans cleanup_countdown(struct scheduler *s) {
  int *c = (int *)s->setup_data;
  if (c[1] > 0) {
    c[1] = c[1] - 1;
    return (struct sched_trans){.s = SCHED_CLEANUP, .why = "counter"};
  } else {
    s->persistent_data = NULL;
    return (struct sched_trans){.s = SCHED_HALT, .why = "counter done"};
  }
}

struct sched_trans bailtick(struct scheduler *s, long _) {
  return (struct sched_trans){.s = SCHED_CLEANUP, .why = "immediate bail"};
}
void noop() {}

void long_setupcleanup() {
  struct scheduler justright, toolong;
  long tns = 100;
  int maxwait = SCHED_SETUP_MAX_EXECS * 4;
  int cjr[] = {SCHED_SETUP_MAX_EXECS, SCHED_CLEANUP_MAX_EXECS};
  int ctl[] = {SCHED_SETUP_MAX_EXECS + 1, SCHED_CLEANUP_MAX_EXECS + 1};
  justright = sched_init(setup_countdown, cleanup_countdown, bailtick, noop,
                         tns, tns, cjr, "justright");
  TEST_ASSERT(sched_get_state(&justright) == SCHED_CONTINUE);
  sched_exec_till_state(&justright, SCHED_CLEANUP, maxwait,
                        "clear out any misc transitions");
  sched_exec_till_state(&justright, SCHED_HALT, maxwait, "test justright");

  toolong = sched_init(setup_countdown, cleanup_countdown, bailtick, noop, tns,
                       tns, ctl, "toolong");
  TEST_ASSERT(sched_get_state(&toolong) == SCHED_ERR);
  toolong.persistent_data = (void *)0xdeadbeef;
  sched_force_transition(&toolong, SCHED_CLEANUP, "test cleanup");
  sched_exec_till_change(&toolong, maxwait, "test toolong");
  TEST_ASSERT(sched_get_state(&toolong) == SCHED_ERR);
  // It should still cleanly exit
  sched_exec_till_state(&toolong, SCHED_HALT, maxwait, "test toolong");
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(long_setupcleanup);
  return UNITY_END();
}
