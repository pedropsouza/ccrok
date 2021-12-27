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
    printf("cleanup %s #%d\n", s->name, c[1]);
    return (struct sched_trans){.s = SCHED_CLEANUP, .why = "counter"};
  } else {
    s->persistent_data = NULL;
    return (struct sched_trans){.s = SCHED_HALT, .why = "counter done"};
  }
}

struct sched_trans bailtick(struct scheduler *s, long _) {
  return (struct sched_trans){.s = SCHED_CLEANUP, .why = "immediate bail"};
}

struct sched_trans errstore(struct scheduler *s) {
  int *errc = (int *)s->setup_data;
  errc[2]++;
  printf("scheduler %s: error #%d\n", s->name, errc[2]);
  fflush(stdout);
  fflush(stderr);
  // Try and make a clean exit
  return (struct sched_trans){.s = SCHED_CLEANUP, .why = "errstore cleanup"};
}

void noop() {}

void long_setupcleanup() {
  struct scheduler justright, toolong;
  long tns = 10000;
  int maxwait = SCHED_SETUP_MAX_EXECS * 4;
  int cjr[] = {SCHED_SETUP_MAX_EXECS, SCHED_CLEANUP_MAX_EXECS, 0};
  int ctl[] = {SCHED_SETUP_MAX_EXECS + 1, SCHED_CLEANUP_MAX_EXECS + 1, 0};
  enum sched_state st;
  justright = sched_init(setup_countdown, cleanup_countdown, bailtick, noop,
                         tns, tns, cjr, "justright");
  sched_set_error_callbk(&justright, errstore);
  st = sched_handover(&justright);
  TEST_ASSERT(st == SCHED_HALT);
  TEST_ASSERT(((int *)justright.setup_data)[2] == 0);

  fflush(stdout);
  fflush(stderr);

  toolong = sched_init(setup_countdown, cleanup_countdown, bailtick, noop, tns,
                       tns, ctl, "toolong");
  sched_set_error_callbk(&toolong, errstore);
  // make sure the cleanup function is actually called
  toolong.persistent_data = (void *)0xdeadbeef;

  st = sched_handover(&toolong);
  sched_debugprint(stdout, &toolong);
  // errstore should've been triggered twice
  TEST_ASSERT(st == SCHED_HALT);
  TEST_ASSERT(((int *)toolong.setup_data)[2] == 2);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(long_setupcleanup);
  return UNITY_END();
}
