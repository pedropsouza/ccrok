#include "memutil.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

// TODO: randomize values?
void test_sarr_alloc_with(void) {
  const size_t e = 1024;
  const size_t l = 128;
  struct sarr a = sarr_alloc_with(e, l, safe_calloc, "test");
  TEST_ASSERT_TRUE(a.head != NULL);
  TEST_ASSERT_TRUE(a.len == l);
  TEST_ASSERT_TRUE(a.elem_s == e);
}

void test_sarr_alloc(void) {
  const size_t e = 1024;
  const size_t l = 128;
  struct sarr a = sarr_alloc(e, l, "test");
  TEST_ASSERT_TRUE(a.head != NULL);
  TEST_ASSERT_TRUE(a.len == l);
  TEST_ASSERT_TRUE(a.elem_s == e);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_sarr_alloc_with);
  RUN_TEST(test_sarr_alloc);
  return UNITY_END();
}
