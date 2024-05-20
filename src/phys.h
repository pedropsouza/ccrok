#ifndef PHYS_HG__
#define PHYS_HG__

#include "fvec.h"
#include <stdio.h>
#include <string.h>

struct physbody {
  struct fvec pos;
  struct fvec vel;
  struct fvec acc;
};

struct physbody physbody_zero();
void physbody_tick(struct physbody *pb);
void physbody_accel(struct physbody *pb, struct fvec v);
//void physbody_force(struct physbody *pb, struct fvec v, long tps);
void physbody_print(struct physbody *pb);

#endif // PHYS_HG__
