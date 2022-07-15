#include "serde.h"

#define IVEC_FMTSTR "QQ"
size_t ivec_pack(pbuf_t *dst, struct ivec *v) {
  return pack(dst, IVEC_FMTSTR, &v->x, &v->y);
}
size_t ivec_unpack(pbuf_t *src, struct ivec *v) {
  return unpack(src, IVEC_FMTSTR, &v->x, &v->y);
}

#define PHYSBODY_FMTSTR "vvvvI" MACROSTR(PHYSBODY_NAME_MAXLEN) "s"
size_t physbody_pack(pbuf_t *dst, struct physbody *pb) {
  return pack(dst, PHYSBODY_FMTSTR, &pb->pos, &pb->vel, &pb->acc, &pb->facc,
              &pb->radius, &pb->name);
}
size_t physbody_unpack(pbuf_t *src, struct physbody *pb) {
  return unpack(src, PHYSBODY_FMTSTR, &pb->pos, &pb->vel, &pb->acc, &pb->facc,
                &pb->radius, &pb->name);
}

#define CROK_PLAYER_FMTSTR "vI" MACROSTR(CROK_PLAYER_NAME_MAXLEN) "sH"
size_t crok_player_pack(pbuf_t *dst, struct crok_player *p) {
  return pack(dst, CROK_PLAYER_FMTSTR, &p->pos, &p->score, &p->name, &p->flags);
}
size_t crok_player_unpack(pbuf_t *src, struct crok_player *p) {
  return unpack(src, CROK_PLAYER_FMTSTR, &p->pos, &p->score, &p->name,
                &p->flags);
}

#define CROK_DISC_FMTSTR "Ip"
size_t crok_disc_pack(pbuf_t *dst, struct crok_disc *d) {
  return pack(dst, CROK_DISC_FMTSTR, &d->flags, &d->pb);
}
size_t crok_disc_unpack(pbuf_t *src, struct crok_disc *d) {
  return unpack(src, CROK_DISC_FMTSTR, &d->flags, &d->pb);
}

#define CROK_STATE_FMTSTR "4P" MACROSTR(CROK_DISCS_TOTAL) "DIIHCC"
size_t crok_state_pack(pbuf_t *dst, struct crok_state *st) {
  return pack(dst, CROK_STATE_FMTSTR, &st->players, &st->discs, &st->round,
              &st->overall_score, &st->gamestage, &st->active_player,
              &st->total_players);
}

size_t crok_state_unpack(pbuf_t *src, struct crok_state *st) {
  return unpack(src, CROK_STATE_FMTSTR, &st->players, &st->discs, &st->round,
                &st->overall_score, &st->gamestage, &st->active_player,
                &st->total_players);
}

size_t pack(pbuf_t *dst, char *fmt, ...) {
  va_list argl;
  va_start(argl, fmt);
  size_t len = pack_unpack(dst, SERDE_MODE_PACK, fmt, argl);
  va_end(argl);
  return len;
}

size_t unpack(pbuf_t *src, char *fmt, ...) {
  va_list argl;
  va_start(argl, fmt);
  size_t len = pack_unpack(src, SERDE_MODE_UNPACK, fmt, argl);
  va_end(argl);
  return len;
}

size_t pack_unpack(pbuf_t *head, char mode, char *fmt, va_list va) {
  union {
    struct crok_state *st;
    struct crok_player *p;
    struct crok_disc *d;
    struct physbody *pb;
    struct ivec *v;
    uint64_t *u64;
    uint32_t *u32;
    uint16_t *u16;
    uint8_t *u8;
    char *str;
  } v;

  size_t len;
  ssize_t valcount = 0;

  size_t size = 0;

  for (; *fmt != '\0'; fmt++) {
    switch (*fmt) {
    case 'C': // byte
      v.u8 = (uint8_t *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          *((uint8_t *)head) = *v.u8;
        } else {
          *v.u8 = *((uint8_t *)head);
        }
        head += sizeof(uint8_t);
        size += sizeof(uint8_t);
        v.u8++;
        valcount--;
      } while (valcount > 0);
      valcount = 0;
      break;
    case 'H': // u16
      v.u16 = (uint16_t *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          *((uint16_t *)head) = htons(*v.u16);
        } else {
          *v.u16 = ntohs(*((uint16_t *)head));
        }
        head += sizeof(uint16_t);
        size += sizeof(uint16_t);
        v.u16++;
        valcount--;
      } while (valcount > 0);
      valcount = 0;
      break;
    case 'I': // u32
      v.u32 = (uint32_t *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          *((uint32_t *)head) = htonl(*v.u32);
        } else {
          *v.u32 = ntohl(*((uint32_t *)head));
        }
        head += sizeof(uint32_t);
        size += sizeof(uint32_t);
        v.u32++;
        valcount--;
      } while (valcount > 0);
      valcount = 0;
      break;
    case 'Q': // u64
      v.u64 = (uint64_t *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
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
      valcount = 0;
      break;
    case 's': // string
      // WARN: won't accept a count on the format, already used for len
      assert(valcount > 0);
      v.str = va_arg(va, char *);
      if (mode == SERDE_MODE_PACK) {
        len = strlen(v.str);
        *((uint16_t *)head) = htons((uint16_t)valcount);
        head += sizeof(uint16_t);
        memcpy(head, v.str, len);
        head += len;
        memset(head, 0, valcount - len);
        head += valcount - len;
        size += valcount + sizeof(uint16_t);
      } else {
        len = ntohs(*(uint16_t *)head);
        head += sizeof(uint16_t);
        memcpy(v.str, head, len);
        head += len;
        size += len + sizeof(uint16_t);
      }
      valcount = 0;
      break;
    case 'v': // ivec
      v.v = (struct ivec *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          len = ivec_pack(head, v.v);
        } else {
          len = ivec_unpack(head, v.v);
        }
        head += len;
        size += len;
        v.v++;
        valcount--;
      } while (valcount > 0);
      valcount = 0;
      break;
    case 'p': // physbody
      v.pb = (struct physbody *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          len = physbody_pack(head, v.pb);
        } else {
          len = physbody_unpack(head, v.pb);
        }
        head += len;
        size += len;
        v.pb++;
        valcount--;
      } while (valcount > 0);
      valcount = 0;
      break;
    case 'P': // player
      v.p = (struct crok_player *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          len = crok_player_pack(head, v.p);
        } else {
          len = crok_player_unpack(head, v.p);
        }
        head += len;
        size += len;
        v.p++;
        valcount--;
      } while (valcount > 0);
      valcount = 0;
      break;
    case 'D': // disc
      v.d = (struct crok_disc *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          len = crok_disc_pack(head, v.d);
        } else {
          len = crok_disc_unpack(head, v.d);
        }
        head += len;
        size += len;
        v.d++;
        valcount--;
      } while (valcount > 0);
      valcount = 0;
      break;
    case 'S': // state
      v.st = (struct crok_state *)va_arg(va, void *);
      do {
        if (mode == SERDE_MODE_PACK) {
          len = crok_state_pack(head, v.st);
        } else {
          len = crok_state_unpack(head, v.st);
        }
        head += len;
        size += len;
        v.st++;
        valcount--;
      } while (valcount > 0);
      valcount = 0;
      break;
    default:
      if (isdigit(*fmt)) {
        valcount = 10 * valcount + (*fmt - '0');
      }
      break;
    }
  }
  return size;
}
