#ifndef FVEC_HG__
#define FVEC_HG__

#include <math.h>
#include <stdio.h>

struct fvec {
  float x, y;
};

struct fvec fvec_zero();
struct fvec fvec_xy(float x, float y);
struct fvec fvec_add(struct fvec a, struct fvec b);
struct fvec fvec_sub(struct fvec a, struct fvec b);
struct fvec fvec_mul(struct fvec a, struct fvec b);
struct fvec fvec_div(struct fvec num, struct fvec den);
float fvec_mag(struct fvec v);
struct fvec fvec_norm(struct fvec v);
void fvec_print(struct fvec v, const char *name);

#endif // FVEC_HG__
