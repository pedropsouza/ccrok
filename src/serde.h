#ifndef SERDE_HG__
#define SERDE_HG__

//#include "crok/config.h"
//#include "crok/globaltypes.h"
//#include "crok/state.h"
//#include "crok/unit.h"
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdalign.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

/*
size_t crok_gstage_serfun(pbuf_t *head, char mode, void* io);

size_t ivec_pack(pbuf_t *dst, struct ivec *v);
size_t ivec_unpack(pbuf_t *src, struct ivec *v);

size_t fvec_pack(pbuf_t *dst, struct fvec *v);
size_t fvec_unpack(pbuf_t *src, struct fvec *v);

size_t physbody_pack(pbuf_t *dst, struct physbody *pb);
size_t physbody_unpack(pbuf_t *src, struct physbody *pb);

size_t crok_player_pack(pbuf_t *dst, struct crok_player *p);
size_t crok_player_unpack(pbuf_t *src, struct crok_player *p);

size_t crok_state_pack(pbuf_t *dst, struct crok_state *st);
size_t crok_state_unpack(pbuf_t *src, struct crok_state *st);

size_t crok_disc_pack(pbuf_t *dst, struct crok_disc *d);
size_t crok_disc_unpack(pbuf_t *src, struct crok_disc *d);

size_t calcsize(char *fmt);
*/

typedef unsigned char pbuf_t;

union ser_t {
    struct crok_state *st;
    struct crok_player *p;
    struct crok_disc *d;
    struct physbody *pb;
    struct ivec *v;
    uint64_t *u64;
    uint32_t *u32;
    uint16_t *u16;
    uint8_t *u8;
    uint32_t *len;
    char *str;
    void *tagged_struct;
  };

typedef size_t (*tagged_struct_serfun)(pbuf_t *head, char mode, void* io);

#define SERDE_MODE_PACK 0
#define SERDE_MODE_UNPACK 1
size_t pack_unpack(pbuf_t *head, char mode, size_t align, char *fmt, va_list va);
size_t pack(pbuf_t *dst, size_t align, char *fmt, ...);
size_t unpack(pbuf_t *src, size_t align, char *fmt, ...);

#endif // SERDE_HG__
