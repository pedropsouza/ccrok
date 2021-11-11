#include "crok/phys.h"

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

void physbody_force(struct physbody *pb, struct ivec v, long tps) {
  struct ivec tmp =
      ivec_div(ivec_add(pb->facc, v), (struct ivec){.x = tps, .y = tps});
  if (tmp.x != 0 || tmp.y != 0) {
    physbody_accel(pb, tmp);
    pb->facc = ivec_sub(pb->facc, tmp);
  }
  pb->facc = ivec_add(pb->facc, ivec_mod(v, (struct ivec){.x = tps, .y = tps}));
}

void physbody_print(struct physbody *pb) {
  printf("physbody \"%s\" -> pos (%ld,%ld), vel (%ld, %ld), acc (%ld, %ld)\n",
         pb->name, pb->pos.x, pb->pos.y, pb->vel.x, pb->vel.y, pb->acc.x,
         pb->acc.y);
}
