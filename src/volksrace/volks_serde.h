#ifndef VOLKS_SERDE_HG__
#define VOLKS_SERDE_HG__

#include "phys.h"
#include "serde.h"
#include "volksrace.h"

size_t fvec_serde(pbuf_t *buf, char mode, void *st);
size_t physbody_serde(pbuf_t *buf, char mode, void *st);

#define SERDE_FUN_DECL(NAME, FIELDS) \
  size_t NAME ## _serde(pbuf_t *buf, char mode, void *st);

SYNCED_STRUCTS(SERDE_FUN_DECL)

#endif
