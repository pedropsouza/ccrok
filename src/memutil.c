#include "memutil.h"

struct sarr sarr_alloc_with(size_t elem_s, size_t s, malloc_fun f,
                            const char *why) {
  struct sarr a = {.head = f(elem_s * s, why), .elem_s = elem_s, .len = s};
  return a;
}

struct sarr sarr_alloc(size_t elem_s, size_t s, const char *why) {
  return sarr_alloc_with(elem_s, s, safe_malloc, why);
}

void *safe_malloc(size_t s, const char *why) {
  void *p = malloc(s);
  if (p == NULL) {
    perror("safe_malloc");
    fprintf(stderr, "thrown while trying: %s", why);
    // Just abort and pretend memory leaks don't exist
    exit(EXIT_FAILURE);
  }
  return p;
}
void *safe_calloc(size_t s, const char *why) {
  void *p = calloc(s, sizeof(uint8_t));
  if (p == NULL) {
    perror("safe_calloc");
    fprintf(stderr, "thrown while trying: %s", why);
    // Just abort and pretend memory leaks don't exist
    exit(EXIT_FAILURE);
  }
  return p;
}
void sarr_free(struct sarr a) { free(a.head); }
