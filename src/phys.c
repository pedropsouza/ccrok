#include "phys.h"

struct physbody physbody_zero() {
  struct physbody pb = {
    .pos = fvec_xy(0,0),
    .vel = fvec_xy(0,0),
    .acc = fvec_xy(0,0),
  };
  return pb;
}

void physbody_tick(struct physbody *pb) {
  // TODO: implement collisions
  pb->vel = fvec_add(pb->vel, pb->acc);
  pb->acc = fvec_xy(0,0);
  pb->pos = fvec_add(pb->pos, pb->vel);
}

void physbody_accel(struct physbody *pb, struct fvec v) {
  pb->acc = fvec_add(pb->acc, v);
}

void physbody_force(struct physbody *pb, struct fvec f, long tps) {
  struct fvec tmp =
      fvec_div(fvec_add(pb->acc, f), (struct fvec){.x = tps, .y = tps});
  physbody_accel(pb, tmp);
}

void physbody_print(struct physbody *pb) {
  printf("pos (%f%f), vel (%f, %f), acc (%f, %f)\n",
         pb->pos.x, pb->pos.y, pb->vel.x, pb->vel.y, pb->acc.x,
         pb->acc.y);
}
