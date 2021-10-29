#include "crok/phys.h"

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

struct physbody physbody_zero(const char *name) {
  return (struct physbody){.name = name,
                           .pos = ivec_zero(),
                           .vel = ivec_zero(),
                           .acc = ivec_zero(),
                           .radius = 0};
}

void physbody_tick(struct physbody *pb) {
  // TODO: implement collisions
  pb->vel = ivec_add(pb->vel, pb->acc);
  pb->acc = ivec_zero();
  pb->pos = ivec_add(pb->pos, pb->vel);
}

void physbody_accel(struct physbody *pb, struct ivec v) {
  pb->acc = ivec_add(pb->acc, v);
}

void physbody_print(struct physbody *pb) {
  printf("physbody \"%s\" -> pos (%ld,%ld), vel (%ld, %ld), acc (%ld, %ld)\n",
         pb->name, pb->pos.x, pb->pos.y, pb->vel.x, pb->vel.y, pb->acc.x,
         pb->acc.y);
}
