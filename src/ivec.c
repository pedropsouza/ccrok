#include "ivec.h"

uint64_t ivec_mag2(struct ivec v) { return v.x * v.x + v.y * v.y; }
// FIXME: int sqrt needed
uint64_t ivec_mag(struct ivec v) { return sqrt(ivec_mag2(v)); }
struct ivec ivec_zero() {
  return (struct ivec){.x = 0, .y = 0};
}
struct ivec ivec_max() {
  return (struct ivec){.x = INT64_MAX, .y = INT64_MAX};
}

struct ivec ivec_add(struct ivec a, struct ivec b) {
  return (struct ivec){.x = a.x + b.x, .y = a.y + b.y};
}
struct ivec ivec_sub(struct ivec a, struct ivec b) {
  return (struct ivec){.x = a.x - b.x, .y = a.y - b.y};
}
struct ivec ivec_mul(struct ivec a, struct ivec b) {
  return (struct ivec){.x = a.x * b.x, .y = a.y * b.y};
}
struct ivec ivec_div(struct ivec a, struct ivec b) {
  return (struct ivec){.x = a.x / b.x, .y = a.y / b.y};
}
void ivec_print(struct ivec v, const char *name) {
  printf("ivec \"%s\" -> %ld , %ld\n", name, v.x, v.y);
}
