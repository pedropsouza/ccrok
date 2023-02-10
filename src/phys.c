#include "phys.h"

struct physbody physbody_zero() {
  struct physbody pb = {
    .pos = ivec_zero(),
    .vel = ivec_zero(),
    .acc = ivec_zero(),
  };
  return pb;
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

// completely broken, needs ratios to be implemented
/*
void physbody_force(struct physbody *pb, struct ivec f, long tps) {
  struct ivec tmp =
      ivec_div(ivec_add(pb->facc, f), (struct ivec){.x = tps, .y = tps});
  // ^ this division is the issue, for sane values it never goes positive
  if (tmp.x != 0 || tmp.y != 0) {
    physbody_accel(pb, tmp);
    pb->facc = ivec_sub(pb->facc, tmp);
  }
  pb->facc = ivec_add(pb->facc, ivec_mod(f, (struct ivec){.x = tps, .y = tps}));
}
*/

void physbody_print(struct physbody *pb) {
  printf("pos (%ld,%ld), vel (%ld, %ld), acc (%ld, %ld)\n",
         pb->pos.x, pb->pos.y, pb->vel.x, pb->vel.y, pb->acc.x,
         pb->acc.y);
}
