#ifndef CROK_PHYS_HG__
#define CROK_PHYS_HG__

#include "crok/config.h"
#include "ivec.h"
#include <stdio.h>
#include <string.h>

struct physbody {
  struct ivec pos;
  struct ivec vel;
  struct ivec acc;
  struct ivec facc;

  // colliders are exclusively circles for now
  uint32_t radius;

  char name[PHYSBODY_NAME_MAXLEN];
};

struct physbody physbody_zero(char *name);
void physbody_tick(struct physbody *pb);
void physbody_accel(struct physbody *pb, struct ivec v);
void physbody_force(struct physbody *pb, struct ivec v, long tps);
void physbody_print(struct physbody *pb);

#endif // CROK_PHYS_HG__
