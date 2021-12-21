#ifndef IVEC_HG__
#define IVEC_HG__

#include <arpa/inet.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define IVEC_SERLEN (sizeof(int64_t) * 2)
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
struct ivec ivec_mod(struct ivec a, struct ivec b);

char *ivec_serialize(const struct ivec *v);
struct ivec ivec_deserialize(char buf[IVEC_SERLEN]);

void ivec_print(struct ivec v, const char *name);

#endif // IVEC_HG__
