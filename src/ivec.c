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
struct ivec ivec_mod(struct ivec a, struct ivec b) {
  return (struct ivec){.x = a.x % b.x, .y = a.y % b.y};
}
void ivec_print(struct ivec v, const char *name) {
  printf("ivec \"%s\" -> %ld , %ld\n", name, v.x, v.y);
}

char *ivec_serialize(const struct ivec *v) {
  static char buf[IVEC_SERLEN];

  char *w_head = buf;
  *((uint32_t *)w_head) = htonl((uint32_t)(0xffffffff & (v->x >> 32)));
  w_head += 4;
  *((uint32_t *)w_head) = htonl((uint32_t)(0xffffffff & (v->x)));
  w_head += 4;
  *((uint32_t *)w_head) = htonl((uint32_t)(0xffffffff & (v->y >> 32)));
  w_head += 4;
  *((uint32_t *)w_head) = htonl((uint32_t)(0xffffffff & (v->y)));

  return buf;
}

struct ivec ivec_deserialize(char buf[IVEC_SERLEN]) {
  char *r_head = buf;
  uint64_t xh = ntohl(*((uint32_t *)r_head));
  r_head += 4;
  uint64_t xl = ntohl(*((uint32_t *)r_head));
  r_head += 4;
  uint64_t yh = ntohl(*((uint32_t *)r_head));
  r_head += 4;
  uint64_t yl = ntohl(*((uint32_t *)r_head));

  return (struct ivec){.x = (int64_t)(xh << 32 | xl),
                       .y = (int64_t)(yh << 32 | yl)};
}
