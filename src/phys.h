#ifndef PHYS_HG__
#define PHYS_HG__

#include "ivec.h"
#include <stdio.h>
#include <string.h>

struct physbody {
  struct ivec pos;
  struct ivec vel;
  struct ivec acc;
};

struct physbody physbody_zero();
void physbody_tick(struct physbody *pb);
void physbody_accel(struct physbody *pb, struct ivec v);
//void physbody_force(struct physbody *pb, struct ivec v, long tps);
void physbody_print(struct physbody *pb);

#endif // PHYS_HG__
