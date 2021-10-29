#ifndef CROK_PHYS_HG__
#define CROK_PHYS_HG__

#include "crok/config.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

struct ivec {
  int64_t x;
  int64_t y;
};

uint64_t ivec_mag2(struct ivec v);
uint64_t ivec_mag(struct ivec v);
struct ivec ivec_zero();
struct ivec ivec_max();

struct ivec ivec_add(struct ivec a, struct ivec b);
struct ivec ivec_sub(struct ivec a, struct ivec b);
struct ivec ivec_mul(struct ivec a, struct ivec b);
struct ivec ivec_div(struct ivec a, struct ivec b);

void ivec_print(struct ivec v, const char *name);

struct physbody {
  struct ivec pos;
  struct ivec vel;
  struct ivec acc;

  // colliders are exclusively circles for now
  uint32_t radius;

  const char *name;
};

struct physbody physbody_zero(const char *name);
void physbody_tick(struct physbody *pb);
void physbody_accel(struct physbody *pb, struct ivec v);
void physbody_print(struct physbody *pb);

#endif // CROK_PHYS_HG__
