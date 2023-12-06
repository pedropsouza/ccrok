#include "volks_serde.h"
#include "serde.h"
#include "volksrace.h"

#define FIELD_FMT(TYPE, NAME, FMTSTR) FMTSTR
#define ARRAY_FMT(TYPE, NAME, NELEM, FMTSTR) FMTSTR

#define FIELD_REF(TYPE, NAME, FMTSTR) &v->NAME,
#define ARRAY_REF(TYPE, NAME, NELEM, FMTSTR) &v->NAME,

// that NULL variadic terminator bothers me, but i can't figure
// out a way to remove only the last comma from the macro expansion
#define SERDE_IMPL(NAME, FIELDS) \
  size_t NAME ## _serde(pbuf_t *buf, char mode, void *vp) { \
    NAME *v = vp; \
    if (mode == SERDE_MODE_SIZEHINT) return sizeof(NAME); \
    return pack_unpack(buf, mode, 8, \
        FIELDS(FIELD_FMT, ARRAY_FMT), \
        FIELDS(FIELD_REF, ARRAY_REF) NULL \
      );\
  } \

SYNCED_STRUCTS(SERDE_IMPL)

size_t fvec_serde(pbuf_t *buf, char mode, void *st) {
  fvec *v = st;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(fvec);
  return pack_unpack(buf, mode, 8, "II", &v->x, &v->y);
}

size_t physbody_serde(pbuf_t *buf, char mode, void *st) {
  physbody *v = st;
  if (mode == SERDE_MODE_SIZEHINT) return sizeof(physbody);
  return pack_unpack(buf, mode, 8, "6I",
    &v->pos.x, &v->pos.y,
    &v->vel.x, &v->vel.y,
    &v->acc.x, &v->acc.y
  );
}

