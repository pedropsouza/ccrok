#include "serde.h"

static size_t pad_align(pbuf_t **head, int alignment) {
  size_t padl = 0;
  while((uintptr_t)*head % alignment != 0) {
    #ifdef DEBUG
    *(*head) = 0xad;
    #endif
    (*head)++;
    padl++;
  }
  return padl;
}

// C - byte;
// H - u16;
// I - u32;
// Q - u64;
// s - string;
// [<kind>] - tagged struct;
size_t pack(pbuf_t *dst, size_t align, char *fmt, ...) {
  va_list argl;
  va_start(argl, fmt);
  size_t len = pack_unpack_v(dst, SERDE_MODE_PACK, align, fmt, argl);
  va_end(argl);
  return len;
}


// C - byte;
// H - u16;
// I - u32;
// Q - u64;
// s - string;
// ? interrogate length of last string;
// [<kind>] - tagged struct;
size_t unpack(pbuf_t *src, size_t align, char *fmt, ...) {
  va_list argl;
  va_start(argl, fmt);
  size_t len = pack_unpack_v(src, SERDE_MODE_UNPACK, align, fmt, argl);
  va_end(argl);
  return len;
}

size_t pack_unpack(pbuf_t *head, char mode, size_t align, char *fmt, ...) {
  va_list argl;
  va_start(argl, fmt);
  size_t len = pack_unpack_v(head, mode, align, fmt, argl);
  va_end(argl);
  return len;
}

size_t pack_unpack_v(pbuf_t *head, char mode, size_t align, char *fmt, va_list va) {
  union ser_t v;

  size_t slen;
  size_t len;
  ssize_t valcount = 0;

  size_t size = 0;

  size += pad_align(&head, align);
  assert((uintptr_t)head % align == 0);
  for (; *fmt != '\0'; fmt++) {
    switch (*fmt) {
    case 'C': // byte
      v.u8 = (uint8_t *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_SIZEHINT) {;}
        else if (mode == SERDE_MODE_PACK) {
          *((uint8_t *)head) = *v.u8;
        } else {
          *v.u8 = *((uint8_t *)head);
        }
        head += sizeof(uint8_t);
        size += sizeof(uint8_t);
        v.u8++;
        valcount--;
      } while (valcount > 0);
      break;
    case 'H': // u16
      size += pad_align(&head, alignof(uint16_t));
      v.u16 = (uint16_t *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_SIZEHINT) {;}
        else if (mode == SERDE_MODE_PACK) {
          *((uint16_t *)head) = htons(*v.u16);
        } else {
          *v.u16 = ntohs(*((uint16_t *)head));
        }
        head += sizeof(uint16_t);
        size += sizeof(uint16_t);
        v.u16++;
        valcount--;
      } while (valcount > 0);
      break;
    case 'I': // u32
      size += pad_align(&head, alignof(uint32_t));
      v.u32 = (uint32_t *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_SIZEHINT) {;}
        else if (mode == SERDE_MODE_PACK) {
          *((uint32_t *)head) = htonl(*v.u32);
        } else {
          *v.u32 = ntohl(*((uint32_t *)head));
        }
        head += sizeof(uint32_t);
        size += sizeof(uint32_t);
        v.u32++;
        valcount--;
      } while (valcount > 0);
      break;
    case 'Q': // u64
      size += pad_align(&head, alignof(uint64_t));
      v.u64 = (uint64_t *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_SIZEHINT) {;}
        else if (mode == SERDE_MODE_PACK) {
          *((uint32_t *)head) = htonl((uint32_t)(0xffffffff & (*v.u64 >> 32)));
          head += sizeof(uint32_t);
          *((uint32_t *)head) = htonl((uint32_t)(0xffffffff & *v.u64));
          head += sizeof(uint32_t);
        } else {
          *v.u64 = (uint64_t)htonl(*((uint32_t *)head)) << 32;
          head += sizeof(uint32_t);
          *v.u64 |= (uint64_t)ntohl(*((uint32_t *)head));
          head += sizeof(uint32_t);
        }
        size += sizeof(uint64_t);
        v.u64++;
        valcount--;
      } while (valcount > 0);
      break;
    case '?': // interrogate length of last string
      if (mode == SERDE_MODE_SIZEHINT) {;}
      else if (mode == SERDE_MODE_UNPACK) {
        v.len = va_arg(va, uint32_t *);
        *v.len = slen;
      } else {
        fprintf(stderr, "'?' in a packing format string, shenanigans will ensue\n");
      }
      break;
    case 's': // string
      // WARN: won't accept a count on the format, already used for length
      size += pad_align(&head, alignof(uint32_t));
      v.str = va_arg(va, char *);
      slen = valcount;
      if (mode == SERDE_MODE_SIZEHINT) {;}
      else if (mode == SERDE_MODE_PACK) {
        *((uint32_t *)head) = htons((uint32_t)slen);
        head += sizeof(uint32_t);
        memcpy(head, v.str, slen);
        head += slen;
      } else {
        slen = ntohs(*(uint32_t *)head);
        head += sizeof(uint32_t);
        memcpy(v.str, head, slen);
        head += slen;
      }
      size += sizeof(uint32_t) + slen;
      break;
    case '[': { // begin tagged struct
      // get the formatter function
      serde_fun serfun = (serde_fun)va_arg(va, serde_fun);
      fmt++; // ignore [ marker
      assert(*fmt != '\0');
      char buf[] = { *fmt, '\0' };
      fmt++; // and the ] too

      if (serfun != NULL) {
        void *io = (void*)va_arg(va, void*);
        do {
          union ser_t kind = (union ser_t)io;
          len = pack_unpack(head, mode, alignof(union ser_t), buf, kind);
          head += len;
          size += len;
          len = serfun(head, mode, io);
          head += len;
          size += len;
          io += serfun(NULL, SERDE_MODE_SIZEHINT, NULL);
          valcount--;
        } while (valcount > 0);
      }
      // ^ This routine is in dire need of an alignment optimization
    } break;
    default:
      if (isdigit(*fmt)) {
        valcount = 10 * valcount + (*fmt - '0');
        continue;
      }
#ifndef SERDE_NOTABLE
      serde_fun f = serializer_table[*fmt];
      if (f != NULL) {
        void *io = (void*)va_arg(va, void*);
        do {
          len = f(head, mode, io);
          head += len;
          size += len;
          io += f(NULL, SERDE_MODE_SIZEHINT, NULL);
          valcount--;
        } while (valcount > 0);
      }
#endif
      break;
    }
    valcount = 0;
  }
  return size;
}
