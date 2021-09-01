#include "fvec.h"

struct fvec fvec_xy(float x, float y) {
  struct fvec v = {.x = x, .y = y};
  return v;
}
struct fvec fvec_add(struct fvec a, struct fvec b) {
  struct fvec v = fvec_xy(a.x + b.x, a.y + b.y);
  return v;
}
struct fvec fvec_sub(struct fvec a, struct fvec b) {
  struct fvec v = fvec_xy(a.x - b.x, a.y - b.y);
  return v;
}
struct fvec fvec_mul(struct fvec a, struct fvec b) {
  struct fvec v = fvec_xy(a.x * b.x, a.y * b.y);
  return v;
}
struct fvec fvec_div(struct fvec num, struct fvec den) {
  struct fvec v = fvec_xy(num.x / den.x, num.y / den.y);
  return v;
}
float fvec_mag(struct fvec v) { return (sqrt(v.x * v.x + v.y * v.y)); }
struct fvec fvec_norm(struct fvec v) {
  float m = fvec_mag(v);
  return fvec_div(v, fvec_xy(m, m));
}
void fvec_print(struct fvec v, const char *name) {
  printf("fvec \"%s\" -> %f , %f\n", name, v.x, v.y);
}
