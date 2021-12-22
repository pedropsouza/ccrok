#include "crok/phys.h"

struct physbody physbody_zero(char name[PHYSBODY_NAME_MAXLEN]) {
  struct physbody pb = {.pos = ivec_zero(),
                        .vel = ivec_zero(),
                        .acc = ivec_zero(),
                        .facc = ivec_zero(),
                        .radius = 0};
  strncpy(pb.name, name, PHYSBODY_NAME_MAXLEN);
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

char *physbody_serialize(const struct physbody *pb) {
  static char buf[PHYSBODY_SERLEN];

  char *w_head = buf;
  memcpy(w_head, ivec_serialize(&pb->pos), IVEC_SERLEN);
  w_head += IVEC_SERLEN;
  memcpy(w_head, ivec_serialize(&pb->vel), IVEC_SERLEN);
  w_head += IVEC_SERLEN;
  memcpy(w_head, ivec_serialize(&pb->acc), IVEC_SERLEN);
  w_head += IVEC_SERLEN;
  memcpy(w_head, ivec_serialize(&pb->facc), IVEC_SERLEN);
  w_head += IVEC_SERLEN;
  *((uint32_t *)w_head) = htonl(pb->radius);
  w_head += sizeof(uint32_t);
  strncpy(w_head, pb->name, PHYSBODY_NAME_MAXLEN);

  return buf;
}
struct physbody physbody_deserialize(char buf[PHYSBODY_SERLEN]) {
  struct physbody pb;

  char *r_head = buf;
  pb.pos = ivec_deserialize(r_head);
  r_head += IVEC_SERLEN;
  pb.vel = ivec_deserialize(r_head);
  r_head += IVEC_SERLEN;
  pb.acc = ivec_deserialize(r_head);
  r_head += IVEC_SERLEN;
  pb.facc = ivec_deserialize(r_head);
  r_head += IVEC_SERLEN;
  pb.radius = ntohl(*((uint32_t *)r_head));
  r_head += sizeof(uint32_t);
  strncpy(pb.name, r_head, PHYSBODY_NAME_MAXLEN);

  return pb;
}
