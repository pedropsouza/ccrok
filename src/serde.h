#ifndef SERDE_HG__
#define SERDE_HG__

#include "crok/config.h"
#include "crok/state.h"
#include "crok/unit.h"
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

typedef unsigned char pbuf_t;

size_t ivec_pack(pbuf_t *dst, struct ivec *v);
size_t ivec_unpack(pbuf_t *src, struct ivec *v);

size_t physbody_pack(pbuf_t *dst, struct physbody *pb);
size_t physbody_unpack(pbuf_t *src, struct physbody *pb);

size_t crok_player_pack(pbuf_t *dst, struct crok_player *p);
size_t crok_player_unpack(pbuf_t *src, struct crok_player *p);

size_t crok_state_pack(pbuf_t *dst, struct crok_state *st);
size_t crok_state_unpack(pbuf_t *src, struct crok_state *st);

size_t crok_disc_pack(pbuf_t *dst, struct crok_disc *d);
size_t crok_disc_unpack(pbuf_t *src, struct crok_disc *d);

#define SERDE_MODE_PACK 0
#define SERDE_MODE_UNPACK 1
size_t pack_unpack(pbuf_t *head, char mode, char *fmt, va_list va);
size_t pack(pbuf_t *dst, char *fmt, ...);
size_t unpack(pbuf_t *src, char *fmt, ...);

#endif // SERDE_HG__
