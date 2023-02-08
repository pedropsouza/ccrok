#ifndef SERDE_HG__
#define SERDE_HG__

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdalign.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

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

typedef size_t (*serde_fun)(pbuf_t *head, char mode, void* io);
#ifndef SERDE_NOTABLE
extern serde_fun serializer_table[256];
#endif

#define SERDE_MODE_PACK 0
#define SERDE_MODE_UNPACK 1
#define SERDE_MODE_SIZEHINT 2
size_t pack_unpack_v(pbuf_t *head, char mode, size_t align, char *fmt, va_list va);
size_t pack_unpack(pbuf_t *head, char mode, size_t align, char *fmt, ...);
size_t pack(pbuf_t *dst, size_t align, char *fmt, ...);
size_t unpack(pbuf_t *src, size_t align, char *fmt, ...);

#endif // SERDE_HG__
